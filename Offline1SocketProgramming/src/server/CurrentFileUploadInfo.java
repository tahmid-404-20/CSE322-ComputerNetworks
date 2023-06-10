package server;

public class CurrentFileUploadInfo {
    int fileId;
    String fileName;
    String fileType;
    long fileSize;

    public CurrentFileUploadInfo(int fileId, String fileName, String fileType, long fileSize) {
        this.fileId = fileId;
        this.fileName = fileName;
        this.fileType = fileType;
        this.fileSize = fileSize;
    }
}
