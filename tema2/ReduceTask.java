import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.RecursiveTask;
import java.util.stream.Collectors;

public class ReduceTask extends RecursiveTask<MapResult> {
    private final String filename;
    private final ArrayList<String> files;
    private final List<MapResult> mapResultsTask;

    public ReduceTask(String filename, List<MapResult> mapTaskList, ArrayList<String> files) {
        this.filename = filename;
        this.mapResultsTask = mapTaskList;
        this.files = files;
    }

    @Override
    protected MapResult compute() {
        List<ReduceTask> reduceTasks = new ArrayList<> ();
        /* The list of files is updated by removing the current file, as we don't want to make
         * any more tasks for it */
        ArrayList<String> newFiles = new ArrayList<> ( files );
        newFiles.remove ( filename );
        /* Create new tasks for the rest of the files */
        if(newFiles.size () >= 1) {
            ReduceTask t = new ReduceTask ( newFiles.get ( 0 ), Tema2.mapResultList.stream ()
                    .filter ( (x) -> x.getFilename ().equals ( newFiles.get ( 0 ) ) )
                    .collect ( Collectors.toList ()), newFiles);
            reduceTasks.add ( t );
            t.fork ();
        }
        for (ReduceTask t : reduceTasks) {
            MapResult res = t.join ();
            /* Store result from task and compute the rank of the file */
            Tema2.reduceResultList.add ( FileInfo.ComputeResult ( res ));
        }
        return MapResult.mergeResults ( mapResultsTask, filename );
    }
}
