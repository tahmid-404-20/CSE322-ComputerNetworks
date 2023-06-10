package util.message;

import java.io.Serializable;

public class SendRequest implements Serializable {
    public String message;

    public SendRequest(String message) {
        this.message = message;
    }
}
