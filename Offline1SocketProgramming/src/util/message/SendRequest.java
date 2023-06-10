package util.message;

import java.io.Serializable;

// used to send file request to other clients via server
public class SendRequest implements Serializable {
    public String message;

    public SendRequest(String message) {
        this.message = message;
    }
}
