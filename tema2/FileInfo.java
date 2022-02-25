import java.util.Map;

public class FileInfo implements Comparable<FileInfo> {
    private final String filename;
    private final String rank;
    private final int maxSize;
    private final int wordsWithMaxSize;

    public FileInfo(String filename, String rank, int maxSixe, int wordsWithMaxSize) {
        this.filename = filename;
        this.rank = rank;
        this.maxSize = maxSixe;
        this.wordsWithMaxSize = wordsWithMaxSize;
    }

    /**
     * @param res - data collected by tasks for a specific file
     * @return - Output object which contains the data for the output file
     */
    public static FileInfo ComputeResult(MapResult res) {
        float wordsNo = 0;
        float sum = 0;
        for(Map.Entry<Integer, Integer> entry : res.getDictionary ().entrySet ()) {
            sum += Tema2.fib.get ( entry.getKey () ).longValueExact () * entry.getValue ();
            wordsNo += entry.getValue ();
        }
        int maxSize =  res.getBiggestWords ().get ( 0 ).length ();
        return new FileInfo ( res.getFilename (), String.format("%.2f", (double)sum / wordsNo), maxSize,
                                                             res.getDictionary ().get ( maxSize ) );
    }

    public String getRank() {
        return rank;
    }

    @Override
    public String toString() {
        return  filename + ',' + rank + ',' + maxSize + ',' + wordsWithMaxSize + '\n';
    }

    @Override
    public int compareTo(FileInfo o) {
        return rank.compareTo (o.rank);
    }
}
