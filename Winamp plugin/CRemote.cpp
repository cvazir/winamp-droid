#include "stdafx.h"
#include "CRemote.h"

CRemote::CRemote(winampGeneralPurposePlugin& p) : ListenSocket{ INVALID_SOCKET }, ClientSocket{ INVALID_SOCKET }, plugin{ &p }, lib(load_media_library())
{
	WSADATA wsaData;

	if (int res = WSAStartup(MAKEWORD(2, 2), &wsaData)) {
		throw res;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints{ 0 };

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	if (int res=getaddrinfo(NULL, DEFAULT_PORT, &hints, &result)) {
		WSACleanup();
		throw res;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		int err = WSAGetLastError();
		freeaddrinfo(result);
		WSACleanup();
		throw err;
	}

	
	if ( bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		int err = WSAGetLastError();
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		throw err;
	}

	freeaddrinfo(result);
	t = new std::thread([&](){ listen_client(); });
}

void CRemote::listen_client()
{
	
	char recvbuf[123];
	//TODO
	int recvbuflen = 123;
	while(listen(ListenSocket, SOMAXCONN) != SOCKET_ERROR) {
		ClientSocket = accept(ListenSocket, NULL, NULL);
		int res;
		if (ClientSocket == INVALID_SOCKET)
			continue;
		else{
			ss = new socketSender(&ClientSocket);
			do{
				res = recv(ClientSocket, recvbuf, recvbuflen, 0);
				if (res == 4)
				{
					send_WM_command(*((int*)recvbuf));
				}
				else if (res == 1)
				{
					switch (*recvbuf){
					case L'p':
						send_playlist();
						break;
					case L'n':
						send_now_playing();
						break;
					case L'm':
						send_media_lib();
						break;
					}
				}
				else if (res==8){
					enqueue(*(int*)recvbuf, *(int*)(recvbuf + 4));
				}
				else if (res == 0)
					closesocket(ClientSocket);
			} while (res > 0);
		}
	}
	closesocket(ListenSocket);
}

void CRemote::enqueue(int artist, int album)
{
	enqueueFileWithMetaStructW s{ 0 };
	if (album!=-1)
		for (song& so : lib.artists[artist].albums[album].songs){
			s.filename = so.filename.c_str();
			s.title = so.title.c_str();
			s.length = so.length;
			SendMessage(plugin->hwndParent, WM_WA_IPC, (WPARAM)&s, IPC_ENQUEUEFILEW);
		}
	else
	{
		for (album = 0; album < lib.artists[artist].albums.size();album++)
			for (song& so : lib.artists[artist].albums[album].songs){
				s.filename = so.filename.c_str();
				s.title = so.title.c_str();
				s.length = so.length;
				SendMessage(plugin->hwndParent, WM_WA_IPC, (WPARAM)&s, IPC_ENQUEUEFILEW);
			}
	}
}

void CRemote::send_media_lib()
{
	std::vector<wchar_t> buff;
	//allocate 10 bytes for header
	for (int i = 0; i < 5; i++)
		buff.push_back(0);

	for (artist& a : lib.artists){
		for (int i = 0, len = a.name.length(); i < len;i++)
			buff.push_back(a.name[i]);
		buff.push_back(0);
		for (album& al : a.albums){
			for (int i = 0, len = al.title.length(); i < len; i++)
				buff.push_back(al.title[i]);
			buff.push_back(0);
		}
		buff.push_back(0);
	}
	//populate header
	int *pdata_length = (int*)&buff[0];
	*pdata_length = buff.size()*sizeof(wchar_t);
	buff[2] = L'm';

	ss->sendData((char*)pdata_length, *pdata_length);
}

void CRemote::send_now_playing()
{
	int pl_pos = SendMessage(plugin->hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
	int song_pos = SendMessage(plugin->hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);//in ms
	int len = SendMessage(plugin->hwndParent, WM_WA_IPC, 2, IPC_GETOUTPUTTIME);//in ms

	int arr[3] = { pl_pos, song_pos/1000, len/1000 };//convert to secs
	ss->sendData((char*)arr, sizeof arr);
}

void CRemote::send_playlist()
{
	//first get playlist info
	//length
	int length = SendMessage(plugin->hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);

	wchar_t* t_pos;
	std::vector<wchar_t> buff;
	//allocate 10 bytes for header
	for (int i = 0; i < 5; i++)
		buff.push_back(0);

	for (int i = 0; i < length; i++)
	{
		t_pos = (wchar_t*)SendMessage(plugin->hwndParent, WM_WA_IPC, i, IPC_GETPLAYLISTTITLEW);
		for (;*t_pos;t_pos++){
			buff.push_back(*t_pos);
		}
		buff.push_back(0);
	}
	//ppopulate header
	int *pdata_length = (int*)&buff[0];
	*pdata_length = buff.size()*sizeof(wchar_t);
	buff[2] = L'p';

	ss->sendData((char*)pdata_length, *pdata_length);
}

library CRemote::load_media_library()
{
	NDEReader n(L"C:/Users/vlad/AppData/Roaming/Winamp/Plugins/ml/main.dat", L"C:/Users/vlad/AppData/Roaming/Winamp/Plugins/ml/main.idx");
	
	return n.load();
}

int CRemote::play()
{
	return send_WM_command(PLAY);
}

int CRemote::pause()
{
	return send_WM_command(PAUSE);
}

int CRemote::next()
{
	return send_WM_command(NEXT);
}

int CRemote::previous()
{
	return send_WM_command(PREVIOUS);
}

int CRemote::vol_up()
{
	return send_WM_command(VOL_UP);
}

int CRemote::vol_down()
{
	return send_WM_command(VOL_DOWN);
}

inline int CRemote::send_WM_command(int command)
{
	return SendMessage(plugin->hwndParent, WM_COMMAND, command, 0);
}

CRemote::~CRemote()
{
	closesocket(ClientSocket);
	closesocket(ListenSocket);
	WSACleanup();

	//delete ss;
	delete t;
}


socketSender::socketSender(SOCKET* sock) : s{ sock }
{
	unsigned int buff;
	int sz = sizeof buff;
	getsockopt(*s, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&buff, &sz);
	max_length = buff;
}

int socketSender::sendData(char* buff, unsigned int len)
{
	int t_sent=0;
	int sent;
	while (len)
	{
		sent = send(*s, buff+t_sent, len > max_length ? max_length : len, 0);
		if (sent <= 0){
			int err = WSAGetLastError();
			std::cout << err;
		}

		len -= sent;
		t_sent += sent;
	}
	return sent;
}

song::song(string filename, string title, string album, string artist, int track_no, int disc_no, int length, int year) : 
	filename(filename), title(title), album(album), artist(artist), track_no(track_no), disc_no(disc_no), length(length), year(year)
{
	
}

song::song(song&& s) : filename(std::move(s.filename)), title(std::move(s.title)), album(std::move(s.album)), artist(std::move(s.artist)), track_no(s.track_no), disc_no(s.disc_no), length(s.length), year(s.year)
{
	
}

album::album(string title, int year) : title(title), songs(), year(year)
{
	
}
album::album(album&& a) : title(std::move(a.title)), songs(std::move(a.songs)), year(a.year)
{
	
}

void album::insert(song *s)
{
	songs.push_back(std::move(*s));
	struct{
		bool operator()(const song& a, const song& b){
			if (a.disc_no < b.disc_no)
				return true;
			else if (a.disc_no==b.disc_no && a.track_no < b.track_no)
				return true;
			return false;
		};
	} p;
	std::sort(songs.begin(), songs.end(), p);
}

artist::artist(string name) : name( name ), albums()
{

}

artist::artist(artist&& a) : name(std::move(a.name)), albums(std::move(a.albums))
{
	
}

void artist::insert(song *s)
{
	for (album &a : albums)
		if (a.title == s->album){
			a.insert(s);
			return;
		}
	albums.push_back(album{ s->album, s->year });
	albums.back().insert(s);
	//sort albums by year
	struct {
		bool operator()(const album& a, const album& b)
		{
			if (a.year < b.year)
				return true;
			else if (a.year == b.year && a.title < b.title)
				return true;
			return false;
		}
	} p;
	std::sort(albums.begin(), albums.end(), p);
}

library::library() : artists{}
{

}

library::library(library&& l) : artists(std::move(l.artists))
{
	
}

void library::insert(song *s)
{
	for (artist& a: artists)
		if (a.name == s->artist){
			a.insert(s);
			return;
		}
	artists.push_back(artist{ s->artist });
	artists.back().insert(s);

	struct{
		bool operator()(const artist& a, const artist& b)
		{
			return a.name < b.name;
		}
	} p;
	std::sort(artists.begin(), artists.end(), p);
}

NDEReader::NDEReader(std::wstring data, std::wstring index) :idx(index, std::ios::binary | std::ios::in), dat(data, std::ios::binary | std::ios::in)
{
	DWORD err = GetLastError();	
	char e[8];
	idx.read(e, 8);
	dat.read(e, 8);
}

library NDEReader::load()
{
	library l;
	int records=0;
	
	//idx.seekg(8);
	idx.read((char*)&records, sizeof records);
	idx.seekg(16);

	dat.seekg(8);

	int offset = 0, rec_ind = 0;
	

	int f_name_id, title_id, album_id, artist_id, track_no_id, disc_no_id, length_id, year_id;

	f_header f_h;
	col_data c_d;
	str_data file,title,album,artist;
	int track, disc, length, year;


	for (int i = 0; i < records; i++)
	{
		idx.read((char*)&offset,sizeof offset);
		idx.read((char*)&rec_ind, sizeof rec_ind);

		dat.seekg(offset);

		if (i == 0) //find ids for the columns we're looking for
		{
			while (offset && dat)
			{
				dat >> f_h;
				dat >> c_d;

				if (c_d.col_name == "filename")
					f_name_id = f_h.col_id;
				else if (c_d.col_name == "title")
					title_id = f_h.col_id;
				else if (c_d.col_name == "artist")
					artist_id = f_h.col_id;
				else if (c_d.col_name == "album")
					album_id = f_h.col_id;
				else if (c_d.col_name == "trackno")
					track_no_id = f_h.col_id;
				else if (c_d.col_name == "disc")
					disc_no_id = f_h.col_id;
				else if (c_d.col_name == "length")
					length_id = f_h.col_id;
				else if (c_d.col_name == "year")
					year_id = f_h.col_id;
				offset = f_h.off_to_next;
				//dat.seekg(offset);
				
			}
		}
		
		else if (i != 1) //skip record 1, it contains unknown stuff
		{
			track = 0;
			disc = 0;
			length = 0;
			year = 0;
			while (offset && dat)
			{
				dat >> f_h;
				if (f_h.col_id == f_name_id){
					dat >> file;
				}
				else if (f_h.col_id == title_id){
					dat >> title;
				}
				else if (f_h.col_id == artist_id){
					dat >> artist;
				}
				else if (f_h.col_id == album_id){
					dat >> album;
				}
				else if (f_h.col_id == length_id){
					dat.read((char*)&length, sizeof length);
				}
				else if (f_h.col_id == track_no_id){
					dat.read((char*)&track,sizeof track);
				}
				else if (f_h.col_id == disc_no_id){
					dat.read((char*)&disc, sizeof disc);
				}
				else if (f_h.col_id == year_id){
					dat.read((char*)&year, sizeof year);
				}
				offset = f_h.off_to_next;
				dat.seekg(offset, std::ios_base::beg);
			}
			if (dat){
				song s{ file.str, title.str, album.str, artist.str, track, disc, length, year };
				l.insert(&s);
			}
		}
	}

	return l;
}