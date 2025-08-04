package ai.guiji.duix;

public class DuixNcnn
{
    public native int alloc(int taskid,int mincalc,int width,int height);
    public native int free(int taskid);
    public native int initPcmex(int maxsize,int minoff,int minblock,int maxblock,int rgb);
    public native int initWenet(String fnwenet);
    public native int initMunet(String fnparam,String fnbin,String fnmask);

    public native long newsession();
    public native int finsession(long sessid);
    public native int consession(long sessid);
    public native int allcnt(long sessid);
    public native int readycnt(long sessid);
    public native int pushpcm(long sessid,byte[] arrbuf,int size, int kind);

    public native int filerst(long sessid,String picfn,String mskfn,
        int[] arrbox,String fgpic,int index, byte[] arrimg,byte[] arrmsk,int imgsize);

    public native int bufrst(long sessid, int[] arrbox,int index, byte[] arrimg,int imgsize);

    public native int fileload(String picfn,String mskfn,int width,int height,
         byte[] arrpic,byte[] arrmsk,int imgsize);

    public native int startgpg(String picfn,String gpgfn);
    public native int stopgpg();
    public native int processmd5(int kind,String infn,String outfn);

    static {
             System.loadLibrary("gjduix");
    }
}
