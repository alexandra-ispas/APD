import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MapResult {
    private final Map<Integer, Integer> dictionary;
    private final ArrayList<String> biggestWords;
    private final String filename;

    public MapResult(Map<Integer, Integer> dictionary, ArrayList<String> biggestWords,
                     String filename) {
        this.dictionary = dictionary;
        this.biggestWords = biggestWords;
        this.filename = filename;
    }

    public MapResult (String filename) {
        this.dictionary = new HashMap<> ();
        this.biggestWords = new ArrayList<> ();
        this.filename = filename;
    }

    public List<String> getBiggestWords() {
        return biggestWords;
    }

    public String getFilename() {
        return filename;
    }

    public Map<Integer, Integer> getDictionary() {
        return dictionary;
    }

    /**
     * Computes the result of a 'Map' Task from a given list of words
     * @param words - from a chunk of text
     * @param filename - processed by task
     * @return - dictionary, and biggest words
     */
    public static MapResult computeResult (List<String> words, String filename) {
        int maxLength = -1;
        Map<Integer, Integer> dictionary = new HashMap<> ();
        ArrayList<String> biggestWords = new ArrayList<> ();

        for(String s : words) {
            Integer res = dictionary.putIfAbsent ( s.length (), 1 );
            if(res != null) {
                dictionary.put ( s.length (), dictionary.get ( s.length () ) + 1);
            }
            if(s.length () > maxLength) {
                maxLength = s.length ();
                biggestWords.clear ();
                biggestWords.add ( s );
            } else if(s.length () == maxLength) {
                biggestWords.add ( s );
            }
        }
        return new MapResult ( dictionary, biggestWords, filename );
    }

    /**
     * Collect data from many tasks and combine it to obtain
     * the results for a specific file
     * @param mapResultsTask - the results from the smaller tasks
     * @param filename - the file which is prcessed
     * @return - the complete analysis of a file
     */
    public static MapResult mergeResults (List<MapResult> mapResultsTask, String filename) {
        int maxLength = -1;
        MapResult out = new MapResult ( filename );
        for (MapResult result : mapResultsTask) {
            /* Get the biggest length in the task */
            int length = result.getBiggestWords ().get ( 0 ).length ();
            if (maxLength < length) {
                /* Update the biggest words */
                out.biggestWords.clear ();
                out.biggestWords.addAll ( result.biggestWords );
                maxLength = length;
            } else if (maxLength == length) {
                /* Complete the list of the biggest words */
                out.biggestWords.addAll ( result.biggestWords );
            }
            for (Map.Entry<Integer, Integer> entry : result.dictionary.entrySet ()) {
                Integer res = out.dictionary.putIfAbsent ( entry.getKey (), entry.getValue () );
                if (res != null) {
                    out.dictionary.put ( entry.getKey (), entry.getValue () +
                            out.dictionary.get ( entry.getKey () ) );
                }
            }
        }
        return out;
    }
}
