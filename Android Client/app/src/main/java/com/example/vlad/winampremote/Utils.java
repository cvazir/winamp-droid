package com.example.vlad.winampremote;

/**
 * Created by vlad on 4/1/15.
 */
public class Utils {

    static final public byte[] intToByte(int d) {
        return new byte[] {
                (byte)d,
                (byte)(d >>> 8),
                (byte)(d >>> 16),
                (byte)(d >>> 24)};
    }
    static public int byteToInt(byte[] d, int off){
        int r=0;
        int t;
        t=0xFF&d[3+off];
        r|=t<<24;
        t=0xFF&d[2+off];
        r|=t<<16;
        t=0xFF&d[1+off];
        r|=t<<8;
        r|=0xFF&d[off];
        return r;
    }
    static public char byteToChar(byte[] d, int off) {
        char c=0;
        c= (char) (c|d[off+1]<<8);
        c= (char) (c | (0xFF&d[off]));
        return c;
    }

    static public char byteToChar(byte[] d){
        return byteToChar(d,0);
    }
    static public int byteToInt(byte [] d){
        return byteToInt(d,0);
    }

}
