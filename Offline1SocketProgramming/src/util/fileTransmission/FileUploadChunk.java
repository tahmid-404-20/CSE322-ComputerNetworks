package util.fileTransmission;

import java.io.Serializable;

public class FileUploadChunk implements Serializable {
    public int fileId;
    public byte[] bytes;
    public int chunkSize;

    public FileUploadChunk(int fileId, byte[] bytes, int chunkSize) {
        this.fileId = fileId;
        this.chunkSize = chunkSize;
        this.bytes = bytes;
    }
}
