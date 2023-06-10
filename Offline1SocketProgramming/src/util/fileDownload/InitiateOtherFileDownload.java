package util.fileDownload;

import java.io.Serializable;

public class InitiateOtherFileDownload implements Serializable {
    public String userName;
    public String fileName;

    public InitiateOtherFileDownload(String userName, String fileName) {
        this.userName = userName;
        this.fileName = fileName;
    }
}
