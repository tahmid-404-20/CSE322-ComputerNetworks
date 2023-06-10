package util.fileDownload;

import java.io.Serializable;

public class FileDownloadPermission implements Serializable {
    public String fileName;
    public String text;

    public FileDownloadPermission(String fileName, String text) {
        this.fileName = fileName;
        this.text = text;
    }
}
