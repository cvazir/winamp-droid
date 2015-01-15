#ifndef CRemote_h
#define CRemote_h

#include <winsock2.h>
#include <ws2tcpip.h>
#include "gen_remote.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>


#define DEFAULT_PORT "37015"

#define PAUSE 40046
#define NEXT 40048
#define PREVIOUS 40044
#define PLAY 40045
#define VOL_UP 40058 
#define VOL_DOWN 40059

using string = std::wstring;

enum f_type{
	Column,
	Index,
	Redirector,
	String,
	Integer,
	Boolean,
	Binary,
	Guid,
	Unknown,
	Float,
	Date_time,
	Length
};

struct f_header
{
	unsigned char col_id = 0, fld_type = 0;
	int dat_size = 0, off_to_next = 0, off_to_prev = 0;

	friend std::ifstream& operator>>(std::ifstream& f, f_header& h)
	{
		f.read((char*)&h.col_id, sizeof h.col_id);
		f.read((char*)&h.fld_type, sizeof h.fld_type);
		f.read((char*)&h.dat_size, sizeof h.dat_size);
		f.read((char*)&h.off_to_next, sizeof h.off_to_next);
		f.read((char*)&h.off_to_prev, sizeof h.off_to_prev);

		return f;
	}
};

struct col_data
{
	unsigned char fld_type, index_unique_values, col_name_size;
	std::string col_name;

	friend std::istream& operator>>(std::ifstream& f, col_data& c)
	{
		f.read((char*)&c.fld_type, sizeof c.fld_type);
		f.read((char*)&c.index_unique_values, sizeof c.index_unique_values);
		f.read((char*)&c.col_name_size, sizeof c.col_name_size);
		c.col_name.resize(c.col_name_size);
		f.read(&c.col_name[0], c.col_name_size);

		return f;
	}
};

struct str_data
{
	unsigned short len;
	std::wstring str;

	friend std::istream& operator>>(std::ifstream& f, str_data& d)
	{
		f.read((char*)&d.len, sizeof d.len);
		d.len -= 2;
		d.str.resize(d.len/2);
		wchar_t a;
		f.read((char*)&a, sizeof a);//skip FF FE bytes which indicate unicode
		f.read((char*)&d.str[0], d.len);

		return f;
	}
};

class song{
public:
	song(string filename, string title, string album, string artist, int trackno,int disc_no, int length, int year);
	song(song&&);

	
//private:
	string filename;
	string title;
	string album;
	string artist;
	int track_no;
	int disc_no;
	int length;
	int year;
};

class album{
public:
	album(string title, int year);
	album(album&&);
	void insert(song *s);

	string title;
	int year;
	std::vector<song> songs;
private:
	
	
};

class artist{
public:
	artist(string name);
	artist(artist&&);
	void insert(song *s);
	string name;
	std::vector<album> albums;
private:
	
	
};

class library{
public:
	library();
	library(library&&);
	void insert(song *s);
	std::vector<artist> artists;
private:
	
};




class socketSender
{
public:
	socketSender(SOCKET*);
	int sendData(char*, unsigned int);

private:
	unsigned int max_length;
	SOCKET *s;
};

class CRemote
{
	
public:
	CRemote(winampGeneralPurposePlugin&);
	~CRemote();

	int play();
	int pause();
	int next();
	int previous();
	int vol_up();
	int vol_down();

private:
	inline int send_WM_command(int command);

	void listen_client();
	void get_playlist();

	void send_playlist();
	void send_now_playing();
	void send_media_lib();

	void enqueue(int artist, int album);

	library load_media_library();

	winampGeneralPurposePlugin *plugin;

	SOCKET ListenSocket;
	SOCKET ClientSocket;

	library lib;

	std::thread *t;
	socketSender *ss;
};


struct index{
	int offset, ind;
};

class NDEReader
{
public:
	NDEReader(std::wstring, std::wstring);
	library load();
private:
	std::ifstream idx, dat;


};

#endif //CRemote_h