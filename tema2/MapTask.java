import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.List;
import java.util.concurrent.RecursiveTask;
import java.util.stream.Collectors;

public class MapTask extends RecursiveTask<MapResult> {
    private final ArrayList<String> files;
    private final int fragmentSize;
    private final int offset;
    private final String filename;
    private final FileChannel channel;

    public MapTask(ArrayList<String> files, int fragmentSize,
                   int offset, String filename, FileChannel channel) {
        this.files = files;
        this.fragmentSize = fragmentSize;
        this.offset = offset;
        this.filename = filename;
        this.channel = channel;
    }

    @Override
    protected MapResult compute() {
        String out = Helper.processText ( fragmentSize, offset, channel );
        /* Get the words from a task */
        List<String> words = Arrays.stream ( out.split ( "[^[a-zA-Z0-9]]") )
                .filter ( x -> x.length () > 0 )
                .collect (Collectors.toList ());

        MapResult result = MapResult.computeResult ( words, filename );
        if(offset != 0) {
            /* the first task created the other ones */
                return result;
        }

        List<MapTask> mapTasks = new ArrayList<> ();
        /* The list of files is updated by removing the current file, as we don't want to make
        * any more tasks for it */
        ArrayList<String> newFiles = new ArrayList<> ( files );
        newFiles.remove ( filename );
        try {
            /* Get the size of the file */
            long bytes = Files.size ( Path.of ( filename ) );
            int i = fragmentSize;
            for (; i < bytes - fragmentSize; i += fragmentSize) {
                /* Crate tasks which contain full chunks of text */
                MapTask t = new MapTask (files, fragmentSize, i, filename, channel );
                mapTasks.add ( t );
                t.fork ();
            }
            /* If there is txt left at the end of the file which was not covered by the other
            * tasks, another one is created*/
            if (i - fragmentSize < bytes) {
                MapTask t = new MapTask ( files, (int) (bytes - i), i, filename, channel );
                mapTasks.add ( t );
                t.fork ();
            }
            /* Create a new task for the next file */
            if (newFiles.size () >= 1) {
                FileInputStream fileInputStream = new FileInputStream(newFiles.get ( 0 ));
                MapTask t = new MapTask ( newFiles, fragmentSize, 0, newFiles.get ( 0 ),
                                            fileInputStream.getChannel () );
                mapTasks.add ( t );
                t.fork ();
            }
        } catch (IOException e) {
            e.printStackTrace ();
        }

        for (MapTask t : mapTasks) {
            MapResult res = t.join ();
            /* Collect the results from the tasks */
            if(res.getDictionary ().size () > 0) {
                Tema2.mapResultList.add ( res );
            }
        }
        return result;
    }
}
