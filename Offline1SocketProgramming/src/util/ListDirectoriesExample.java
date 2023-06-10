package util;

import java.io.File;

public class ListDirectoriesExample {
    public static void main(String[] args) {
        String directoryPath = "files"; // Replace with the actual directory path

        // Create a File object representing the directory
        File directory = new File(directoryPath);

        // Check if the provided path points to a directory
        if (directory.isDirectory()) {
            // Get the list of files and directories in the given directory
            File[] files = directory.listFiles();

            if (files != null) {
                // Iterate over the files and directories
                for (File file : files) {
                    if (file.isDirectory()) {
                        System.out.println("Directory: " + file.getName());
                    }
                }
            }
        } else {
            System.out.println("The specified path is not a directory.");
        }
    }
}

