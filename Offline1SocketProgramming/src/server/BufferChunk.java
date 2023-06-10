package server;

public class BufferChunk {
    byte[] bytes;
    int chunkSize;

    public BufferChunk(byte[] bytes, int chunkSize) {
        this.bytes = bytes;
        this.chunkSize = chunkSize;
    }
}
