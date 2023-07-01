package client;

import util.BufferChunk;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class CurrentFileDownloadInfo {
    String fileName;
    List<BufferChunk> bufferChunks;

    public CurrentFileDownloadInfo(String fileName) {
        this.fileName = fileName;
        this.bufferChunks = new ArrayList<>();
    }

    public void addChunk(byte[] bytes, int chunkSize) {
        this.bufferChunks.add(new BufferChunk(bytes, chunkSize));
    }

    public void saveFile() {
        try {
            FileOutputStream fos = new FileOutputStream("downloads/"+fileName);

            for (BufferChunk bufferChunk : bufferChunks) {
                fos.write(bufferChunk.bytes, 0, bufferChunk.chunkSize);
            }
            fos.close();
        } catch (IOException e) {
            System.out.println("Exception in CurrentFileDownloadInfo.saveFile() : " + e);
        }
    }
}
