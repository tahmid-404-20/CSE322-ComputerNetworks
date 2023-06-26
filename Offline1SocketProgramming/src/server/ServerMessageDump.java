package server;

import util.message.Message;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ServerMessageDump {
    volatile long noOfMessages;
    Map<Long, Message> messageHashMap;
    Map<String, ClientInfo> clientInfoMap;

    public ServerMessageDump(Map<String, ClientInfo> clientInfoMap) {
        noOfMessages = 0;
        messageHashMap = new ConcurrentHashMap<>();
        this.clientInfoMap = clientInfoMap;
    }

    public void addMessage(Message message) {
        synchronized (this) {
            noOfMessages++;
        }
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
