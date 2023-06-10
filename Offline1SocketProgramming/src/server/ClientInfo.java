package server;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class ClientInfo {
    String name;


    List<String> lookUpOwnFiles() {
        List<String> fileNames = new ArrayList<>();

        // private files
        String directoryPathPrivate = "files/" + name + "/private/";
        File directory = new File(directoryPathPrivate);
        File[] files = directory.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isFile()) {
                    fileNames.add(file.getName() + " (private)");
                }
            }
        }

        // public files
        String directoryPathPublic = "files/" + name + "/public/";
        directory = new File(directoryPathPublic);
        files = directory.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isFile()) {
                    fileNames.add(file.getName() + " (public)");
                }
            }
        }
        return fileNames;
    }
}
