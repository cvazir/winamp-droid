package com.example.vlad.winampremote;

import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.ActionProvider;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseExpandableListAdapter;
import android.widget.ExpandableListView;
import android.widget.ListView;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;


public class MainActivity extends Activity {

    NetworkThread t;
    InputProcessingThread t2;
    Timer timer;
    List<String> playlist;
    final static int port=37015;

    enum commands{
        play(40045),
        pause(40046),
        previous(40044),
        next(40048),
        volUp(40058),
        volDown(40059);


        int c;

        commands(int i){
            c=i;
        }};

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        t=new NetworkThread("10.0.0.4",port);
        t2=new InputProcessingThread(t,this);
        ExpandableListView elv=(ExpandableListView)findViewById(R.id.ExpandableListView);
        registerForContextMenu(elv);
        t.start();
        t2.start();
        timer=new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
//                t.getNowPlaying();
            }
        },1000,1000);
    }

    @Override
    public void onCreateContextMenu(ContextMenu contextMenu, View view, ContextMenu.ContextMenuInfo contextMenuInfo) {
        contextMenu.add("enqueue".subSequence(0,7));
    }

    public boolean onContextItemSelected(MenuItem menuItem) {

        ExpandableListView.ExpandableListContextMenuInfo info =(ExpandableListView.ExpandableListContextMenuInfo) menuItem.getMenuInfo();

        int i = ExpandableListView.getPackedPositionGroup(info.packedPosition);
        int i2 = ExpandableListView.getPackedPositionChild(info.packedPosition);
        byte[] arr=new byte [8];
        byte[] tmp;

        tmp=Utils.intToByte(i);
        for(int j=0;j<4;j++)
            arr[j]=tmp[j];

        tmp=Utils.intToByte(i2);
        for(int j=0;j<4;j++)
            arr[j+4]=tmp[j];

        t.sendCommand(arr);

        return true;
    }


    @Override
    protected void onResume() {
        super.onResume();
        timer=new Timer();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                t.getNowPlaying();
            }
        },1000,1000);
        t.connect();
    }

    @Override
    protected void onPause() {
        super.onPause();
        timer.cancel();
        t.disconnect();
    }

    @Override
    protected void onStop() {
        super.onStop();
        timer.cancel();
        t.disconnect();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    protected void sendCommand(int command){
        t.sendCommand(command);
    }
    protected void sendCommand(byte[] command){
        t.sendCommand(command);
    }

    public void onClickPlay(View v){
        sendCommand(commands.pause.c);
    }
    public void onClickNext(View v){
        sendCommand(commands.next.c);
    }
    public void onClickPrevious(View v){
        sendCommand(commands.previous.c);
    }
    public void onClickVolUp(View v){
        sendCommand(commands.volUp.c);
    }
    public void onClickVolDown(View v){
        sendCommand(commands.volDown.c);
    }
    public void onClickGetPlaylist(View v){
        sendCommand(new byte[]{'p'});
    }
    public void onClickGetMediaLib(View v){
        sendCommand(new byte[]{'m'});
    }
    class NetworkThread extends Thread{
        private Socket s;
        private BlockingQueue<byte[]> in;
        private BlockingQueue<byte[]> out;

        private InputStream sin;
        private OutputStream sout;

        final byte[] connect={'c'},disconnect={'d'};
        private String address;
        private int port;

        NetworkThread(String addr,int port) {
            this.address=addr;
            this.port=port;

            in=new ArrayBlockingQueue<byte[]>(10);
            out=new ArrayBlockingQueue<byte[]>(10);
        }

        @Override
        public void run() {
            super.run();
            byte[] c;
            byte[] r;
            int res;
            while (true) {
                try {
                    c = in.take();
                    if(c==connect){
                        if(!int_connect())
                            return;
                    } else if(c==disconnect) {
                        int_disconnect();
                    } else if (c[0]=='p'){
                        //send request for playlist
                        sout.write(c);
                        sout.flush();

                        byte[] t=new byte[4];
                        int tRead=0;
                        //wait for response
                        tRead+=sin.read(t,0,t.length);

                        if(tRead<4)
                            continue;
                        //get total number of bytes we'll be getting
                        int playlist_len=Utils.byteToInt(t);
                        //get remaining data to receive
                        byte[] buffer=new byte[playlist_len];
                        while(tRead<playlist_len)
                            tRead+=sin.read(buffer,tRead,playlist_len-tRead);
                        out.add(buffer);
                    } else if(c[0]=='n'){
                        //send request for now playing song
                        if(sout==null)
                            continue;
                        sout.write(c);
                        sout.flush();
                        byte[] temp=new byte[12];
                        sin.read(temp);
                        out.add(temp);
                    } else if(c[0]=='m'){
                        //send request for media library
                        sout.write(c);
                        sout.flush();

                        byte[] t=new byte[4];
                        int tRead=0;
                        //wait for response
                        tRead+=sin.read(t,0,t.length);

                        if(tRead<4)
                            continue;
                        //get total number of bytes we'll be getting
                        int mediaLib_length=Utils.byteToInt(t);
                        //get remaining data to receive
                        byte[] buffer=new byte[mediaLib_length];
                        while(tRead<mediaLib_length)
                            tRead+=sin.read(buffer,tRead,mediaLib_length-tRead);
                        out.add(buffer);
                    } else if (c.length==4 || c.length==8){
                        sout.write(c);
                        sout.flush();
                    }
                } catch (InterruptedException ex){

                } catch (IOException ex){

                }
            }

        }

        private boolean int_connect(){
            try {
                s=new Socket(address,port);
                sin=s.getInputStream();
                sout=s.getOutputStream();
                return true;
            } catch (IOException e) {
                return false;
            }
        }
        private boolean int_disconnect(){
            try {
                s.close();
                sin=null;
                sout=null;
                return true;
            } catch (IOException e) {
                return false;
            }
        }


        public void sendCommand(byte[] d){
            in.offer(d);
        }
        public void sendCommand(int d){
            sendCommand(Utils.intToByte(d));
        }



        public void disconnect() {
            t.sendCommand(t.disconnect);
        }

        public void connect() {
            t.sendCommand(t.connect);
        }

        public void getNowPlaying(){
            t.sendCommand(new byte[]{'n'});
        }

        public void getMediaLib(){
            t.sendCommand(new byte[]{'m'});
        }
    }

    class InputProcessingThread extends Thread{
        NetworkThread t;
        Activity parent;

        InputProcessingThread(NetworkThread t,Activity p) {
            this.t = t;
            parent=p;
        }

        @Override
        public void run() {
            super.run();
            while(true){
                byte[] job;
                char c;
                byte[] tmp=new byte[2];
                try {
                    job=t.out.take();
                    if(job.length==12){//update now playing info
                        byte[] temp=new byte[4];

                        temp[0]=job[0];
                        temp[1]=job[1];
                        temp[2]=job[2];
                        temp[3]=job[3];
                        final int pl_pos=Utils.byteToInt(temp);

                        temp[0]=job[4];
                        temp[1]=job[5];
                        temp[2]=job[6];
                        temp[3]=job[7];
                        int song_pos=Utils.byteToInt(temp);
                        int t=song_pos%60;
                        final String spos=""+song_pos/60+":"+(t<10?("0"+t):t);

                        temp[0]=job[8];
                        temp[1]=job[9];
                        temp[2]=job[10];
                        temp[3]=job[11];
                        int song_len=Utils.byteToInt(temp);
                        t=song_len%60;
                        final String slen=""+song_len/60+":"+(t<10?("0"+t):t);

                        parent.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                if(playlist==null)
                                    return;
                                TextView tv = (TextView) findViewById(R.id.TextView);
                                String s = playlist.get(pl_pos);
                                s += " " + spos + "/" + slen;
                                tv.setText(s.toCharArray(), 0, s.length());
                            }
                        });

                    }  else if(Utils.byteToChar(job, 4)=='p'){
                        StringBuilder sb = new StringBuilder();
                        final List<String> names = new ArrayList<String>(20);
                        int i = 10;
                        for (; i < job.length; i += 2) {
                            tmp[0] = job[i];
                            tmp[1] = job[i + 1];
                            c = Utils.byteToChar(tmp);
                            if (c == 0) {
                                names.add(sb.toString());
                                sb = new StringBuilder();
                            } else
                                sb.append(c);
                        }


                        parent.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                playlist=names;
                                ArrayAdapter<String> arrayAdapter = new ArrayAdapter<String>(parent, R.layout.row_layout, R.id.label, names);
                                findViewById(R.id.ExpandableListView).setVisibility(View.GONE);
                                ((ListView) findViewById(R.id.listView)).setAdapter(arrayAdapter);
                                findViewById(R.id.listView).setVisibility(View.VISIBLE);

                            }
                        });
                    } else if (Utils.byteToChar(job, 4)=='m'){
                        StringBuilder sb=new StringBuilder();
                        final List<String> artist = new ArrayList<String>(20);
                        final HashMap<String, List<String>> albums= new HashMap<String, List<String>>();
                        List<String> aa=null;
                        boolean album=false;
                        int nulls=0;
                        int i = 10;
                        for (; i < job.length; i += 2) {
                            tmp[0] = job[i];
                            tmp[1] = job[i + 1];
                            c = Utils.byteToChar(tmp);
                            if (c == 0) {
                                nulls++;
                                if(album){
                                    if(nulls==2)
                                        album=false;
                                    else
                                        aa.add(sb.toString());
                                }
                                else {
                                    artist.add(sb.toString());
                                    aa=new ArrayList<String>();
                                    albums.put(artist.get(artist.size()-1),aa);
                                    album=true;
                                }
                                sb = new StringBuilder();
                            } else {
                                nulls=0;
                                sb.append(c);
                            }
                        }
                        parent.runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                BaseExpandableListAdapter adapter=new BaseExpandableListAdapter() {

                                    HashMap<String, List<String>> data=albums;
                                    List<String> head=artist;
                                    Context context=parent;

                                    @Override
                                    public int getGroupCount() {
                                        return data.size();
                                    }

                                    @Override
                                    public int getChildrenCount(int i) {
                                        return data.get(head.get(i)).size();
                                    }

                                    @Override
                                    public Object getGroup(int i) {
                                        return head.get(i);
                                    }

                                    @Override
                                    public Object getChild(int i, int i2) {
                                        return data.get(head.get(i)).get(i2);
                                    }

                                    @Override
                                    public long getGroupId(int i) {
                                        return i;
                                    }

                                    @Override
                                    public long getChildId(int i, int i2) {
                                        return i2;
                                    }

                                    @Override
                                    public boolean hasStableIds() {
                                        return false;
                                    }

                                    @Override
                                    public View getGroupView(int i, boolean b, View view, ViewGroup viewGroup) {
                                        String headerTitle = (String) getGroup(i);
                                        if (view == null) {
                                            LayoutInflater inflater = (LayoutInflater) this.context
                                                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                                            view = inflater.inflate(R.layout.expandable_row_layout, null);
                                        }

                                        TextView lblListHeader = (TextView) view
                                                .findViewById(R.id.lblListHeader);
                                        lblListHeader.setTypeface(null, Typeface.BOLD);
                                        lblListHeader.setText(headerTitle);

                                        return view;
                                    }

                                    @Override
                                    public View getChildView(int i, int i2, boolean b, View view, ViewGroup viewGroup) {
                                        final String childText = (String) getChild(i, i2);

                                        if (view == null) {
                                            LayoutInflater inflater = (LayoutInflater) this.context
                                                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
                                            view = inflater.inflate(R.layout.row_layout, null);
                                        }

                                        TextView txtListChild = (TextView) view
                                                .findViewById(R.id.label);

                                        txtListChild.setText(childText);
                                        return view;
                                    }

                                    @Override
                                    public boolean isChildSelectable(int i, int i2) {
                                        return true;
                                    }
                                };
                                findViewById(R.id.listView).setVisibility(View.GONE);
                                ExpandableListView elv=(ExpandableListView)findViewById(R.id.ExpandableListView);
                                elv.setAdapter(adapter);
                                findViewById(R.id.ExpandableListView).setVisibility(View.VISIBLE);
                            }
                        });
                    }
                } catch (InterruptedException e) {

                }

            }
        }
    }
}
