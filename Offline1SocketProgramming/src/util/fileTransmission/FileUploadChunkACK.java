package util.fileTransmission;

import java.io.Serializable;

public class FileUploadChunkACK implements Serializable {
    public int fileId;
    public int chunkSize;

    public FileUploadChunkACK(int fileId, int chunkSize) {
        this.fileId = fileId;
        this.chunkSize = chunkSize;
    }
}
