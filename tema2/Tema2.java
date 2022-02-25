import java.io.*;
import java.math.BigInteger;
import java.nio.channels.FileChannel;
import java.util.*;
import java.util.concurrent.ForkJoinPool;
import java.util.stream.Collectors;

public class Tema2 {
    static int fragmentSize;
    static ArrayList<String>  files = new ArrayList<> ();
    static List<MapResult> mapResultList = new ArrayList<> (); // the results from the 'Map' operations
    static List<FileInfo> reduceResultList = new ArrayList<> (); // the results from the 'Reduce' operations
    static List<BigInteger> fib = Helper.fibonacci ( 100); // List of the first 100 fibonacci
                                                                                    //numbers

    public static void main(String[] args) throws IOException {
        if (args.length < 3) {
            System.err.println ( "Usage: Tema2 <workers> <in_file> <out_file>" );
            return;
        }
        /* Read data from the input file */
        BufferedReader reader = new BufferedReader ( new FileReader ( args[1] ) );
        fragmentSize = Integer.parseInt ( reader.readLine () );
        int filesNo = Integer.parseInt ( reader.readLine () );
        for (int i = 0; i < filesNo; i++) {
            files.add ( reader.readLine () );
        }
        reader.close ();

        /* Create the first pool of tasks. It is used for the 'Map' operations */
        ForkJoinPool fjpMap = new ForkJoinPool ( Integer.parseInt ( args[0] ) );
        FileInputStream fileInputStream = new FileInputStream(files.get ( 0 ));
        FileChannel channel = fileInputStream.getChannel();
        MapResult res = fjpMap.invoke ( new MapTask ( files, fragmentSize, 0, files.get ( 0 ),
                                                                                        channel) );
        mapResultList.add ( res );
        fjpMap.shutdown ();
        channel.close ();
        fileInputStream.close ();

        /* The second pool of tasks, used for the 'Reduce' operations */
        ForkJoinPool fjpReduce = new ForkJoinPool ( Integer.parseInt ( args[0] ) );
        /* The task receives all the data for a specific file */
        res = fjpReduce.invoke ( new ReduceTask (files.get ( 0 ), mapResultList.stream ()
                .filter ( (x) -> x.getFilename ().equals ( files.get ( 0 ) ) )
                .collect (Collectors.toList ()), files) );
        fjpReduce.shutdown ();
        /* Store result from task */
        reduceResultList.add ( FileInfo.ComputeResult ( res ));

        /* Sort results by rank */
        reduceResultList.sort ((o1, o2) -> -Float.compare ( Float.parseFloat (o1.getRank ()),
                Float.parseFloat (o2.getRank ()) ) );

        /* Compute the text for the output file */
        StringBuilder output = new StringBuilder ();
        for(FileInfo o : reduceResultList) {
            output.append ( o.toString ().substring ( 12 ) );
        }
        try{
            FileWriter myWriter = new FileWriter(args[2]);
            myWriter.write ( String.valueOf ( output ) );
            myWriter.close ();
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
        }
    }
}
