package util.fileDownload;

import java.io.Serializable;

public class InitiateSelfFileDownload implements Serializable {
    public String fileName;
    public String fileType;

    public InitiateSelfFileDownload(String fileName, String fileType) {
        this.fileName = fileName;
        this.fileType = fileType;
    }
}
