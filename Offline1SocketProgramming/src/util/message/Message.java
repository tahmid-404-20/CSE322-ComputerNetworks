package util.message;

import java.io.Serializable;

public class Message implements Serializable {
    public long requestId;
    public String message;
    public String sender;

    public Message(long requestId, String message, String sender) {
        this.requestId = requestId;
        this.message = message;
        this.sender = sender;
    }

    public Message(String message, String sender) {
        this.message = message;
        this.sender = sender;
    }
}
