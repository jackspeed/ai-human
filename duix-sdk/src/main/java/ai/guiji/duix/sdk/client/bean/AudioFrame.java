package ai.guiji.duix.sdk.client.bean;

public class AudioFrame {

    public AudioFrame(byte[] buffer, int size){
        this.buffer = buffer;
        this.size = size;
    }

    public AudioFrame(boolean completeEmptyFrame){
        this.completeEmptyFrame = completeEmptyFrame;
    }

    public boolean completeEmptyFrame;

    public byte[] buffer;
    public int size;

    @Override
    public String toString() {
        return "AudioFrame{" +
                "completeEmptyFrame=" + completeEmptyFrame +
                '}';
    }
}
