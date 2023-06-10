package util.lookUps;

import java.io.Serializable;

// 4 type of lookUp requests: OwnFiles, Other's files, Other clientNames with activeStatus, UnreadMessages
public class LookUpRequest implements Serializable {
    public String requestText;

    public LookUpRequest(String requestText) {
        this.requestText = requestText;
    }
}
