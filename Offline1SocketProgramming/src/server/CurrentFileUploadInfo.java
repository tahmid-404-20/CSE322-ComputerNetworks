package server;

public class CurrentFileUploadInfo {
    int fileId;
    String fileName;
    String fileType;
    long fileSize;
    public boolean isResponseToRequest;
    public long requestId;

    public CurrentFileUploadInfo(int fileId, String fileName, String fileType, long fileSize) {
        this.fileId = fileId;
        this.fileName = fileName;
        this.fileType = fileType;
        this.fileSize = fileSize;
        this.isResponseToRequest = false;
    }

    public CurrentFileUploadInfo(int fileId, String fileName, String fileType, long fileSize, long requestId) {
        this.fileId = fileId;
        this.fileName = fileName;
        this.fileType = fileType;
        this.fileSize = fileSize;
        this.requestId = requestId;
        this.isResponseToRequest = true;
    }
}
