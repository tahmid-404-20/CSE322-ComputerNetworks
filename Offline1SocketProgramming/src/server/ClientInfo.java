package server;

import util.message.Message;

import java.util.ArrayList;
import java.util.List;

public class ClientInfo {
    String name;
    List<Message> unreadMessages;
    boolean isOnline;

    public ClientInfo(String name) {
        this.name = name;
        unreadMessages = new ArrayList<>();
        isOnline = false;
    }

    public void addMessage(Message message) {
        unreadMessages.add(message);
    }

    public List<Message> getUnreadMessages() {
        return unreadMessages;
    }
}
