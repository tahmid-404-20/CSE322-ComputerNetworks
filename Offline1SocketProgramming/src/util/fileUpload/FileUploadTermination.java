package util.fileUpload;

import java.io.Serializable;

public class FileUploadTermination implements Serializable {
    public int fileId;
    public String text;

    public FileUploadTermination(int fileId, String text) {
        this.fileId = fileId;
        this.text = text;
    }
}
