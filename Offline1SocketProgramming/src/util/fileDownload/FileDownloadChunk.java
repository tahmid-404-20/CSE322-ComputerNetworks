package util.fileDownload;

import java.io.Serializable;

public class FileDownloadChunk implements Serializable {
    public String fileName;
    public byte[] bytes;
    public int chunkSize;

    public FileDownloadChunk(String fileName, byte[] bytes, int chunkSize) {
        this.fileName = fileName;
        this.chunkSize = chunkSize;
        this.bytes = bytes;
    }

}
