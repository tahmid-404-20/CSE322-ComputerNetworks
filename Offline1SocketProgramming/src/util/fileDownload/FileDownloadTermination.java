package util.fileDownload;

import java.io.Serializable;

public class FileDownloadTermination implements Serializable {
    public String fileName;
    public String text;

    public FileDownloadTermination(String fileName, String text) {
        this.fileName = fileName;
        this.text = text;
    }
}
