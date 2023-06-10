package server;

import util.message.Message;

import java.util.HashMap;

public class ServerMessageDump {
    long noOfMessages;
    HashMap<Long, Message> messageHashMap;
    HashMap<String, ClientInfo> clientInfoMap;

    public ServerMessageDump(HashMap<String, ClientInfo> clientInfoMap) {
        noOfMessages = 0;
        messageHashMap = new HashMap<>();
        this.clientInfoMap = clientInfoMap;
    }

    public void addMessage(Message message) {
        noOfMessages++;
        message.requestId = noOfMessages;
        messageHashMap.put(noOfMessages, message);

        // broadcast message to all clients except the sender
        for(String clientName : clientInfoMap.keySet()) {
            if(!clientName.equals(message.sender)) {
                clientInfoMap.get(clientName).addMessage(message);
            }
        }
    }

    public Message getMessage(long requestId) {
        return messageHashMap.get(requestId);
    }
}
