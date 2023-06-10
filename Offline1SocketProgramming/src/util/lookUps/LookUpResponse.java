package util.lookUps;

import java.io.Serializable;

public class LookUpResponse implements Serializable {
    public String requestDetails;
    public String responseText;

    public LookUpResponse(String requestDetails, String responseText) {
        this.requestDetails = requestDetails;
        this.responseText = responseText;
    }
}
