package util;

import java.io.Serializable;

public class BufferChunk implements Serializable {
    public byte[] bytes;
    public int chunkSize;

    public BufferChunk(byte[] bytes, int chunkSize) {
        this.bytes = bytes;
        this.chunkSize = chunkSize;
    }
}

