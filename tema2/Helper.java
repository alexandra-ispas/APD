import java.io.IOException;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class Helper {
    /**
     *
     * @param limit - the size of the List
     * @return - the first 'limit' fibonacci numbers
     */
    public static List<BigInteger> fibonacci(int limit) {
        return Stream.iterate ( new BigInteger[] {BigInteger.ZERO, BigInteger.ONE},
                        t -> new BigInteger[] {t[1], t[0].add ( t[1] )} )
                .limit ( limit )
                .map ( n -> n[1] )
                .collect ( Collectors.toList () );
    }

    public static String fragmentEndsInTheMiddleOfAWord(String out, FileChannel channel,
               int offset, int fragmentSize) throws IOException {
        /* Read another block of bytes in order to try to find the end of the word*/
        ByteBuffer str1 = ByteBuffer.allocate ( 30 );
        channel.read ( str1, offset + fragmentSize + 1 );
        String ch = new String ( str1.array () );

        int pos = ch.indexOf ( " " );
        if (pos != -1) {
            /* The end of the word is found, the remaining letters are added to the current task*/
            out += ch.substring ( 0, pos );
        } else {
             /* A separator was not found, we consider that the word was the last one
             * in that file and add the remaining valid letters to our word */
            for (int i = 0; i < ch.length (); i++) {
                if (!Character.isLetter ( ch.charAt ( i ) )) {
                    out += ch.substring ( 0, i );
                    break;
                }
            }
        }
        return out;
    }

    /**
     *
     * @param out - the resulted text
     * @param offset - the offset from the beginning of the file
     * @param channel - the reading channel
     * @param fragmentSize - the size of the chunk of text
     * @return - the text assigned for this task, having the last word completed
     *           in case it had been damaged by fragmenting
     * @throws IOException
     */
    public static String getCheckEndOfChunk(String out, int offset, FileChannel channel, int fragmentSize)
            throws IOException {
        if (Character.isLetter ( out.charAt ( out.length () - 1 ) ) &&
                Character.isLetter ( out.charAt ( out.length () - 2) )) {
            out = fragmentEndsInTheMiddleOfAWord ( out, channel, offset, fragmentSize );
        } else {
            out = out.substring ( 0, out.length () - 1 );
        }
        return out;
    }

    /**
     *
     * @param out - the resulted text
     * @return - the text assigned for this task, having the first word completed
     *           in case it had been damaged by fragmenting
     */
    public static String checkBeginningOfChunk (String out) {
        if(Character.isLetter ( out.charAt (0) ) && Character.isLetter ( out.charAt (1) )) {
            int pos = out.indexOf ( " " );
            if(pos == -1) {
               return  "" ;
            } else {
                return out.substring (pos + 1);
            }
        } else {
            return out.substring ( 1 );
        }
    }

    public static String processText(int fragmentSize, int offset, FileChannel channel) {
        String out = null;
        try {
            if (fragmentSize == Tema2.fragmentSize && offset == 0) {
                /* When the task gets the first chunk of text from a file */
                ByteBuffer str = ByteBuffer.allocate ( fragmentSize + 1 );
                channel.read ( str, offset );
                out = Helper.getCheckEndOfChunk ( new String ( str.array ()), offset, channel, fragmentSize );
            } else if(fragmentSize == Tema2.fragmentSize && offset > 0) {
                /* When the task gets a chunk of text from the middle of the file */
                ByteBuffer str = ByteBuffer.allocate (fragmentSize + 2);
                channel.read ( str, offset - 1);
                out = Helper.getCheckEndOfChunk ( new String ( str.array ()), offset, channel, fragmentSize );
                out = Helper.checkBeginningOfChunk (out);
            } else if ( fragmentSize < Tema2.fragmentSize) {
                /* When the task gets the last chunk of text from a file */
                ByteBuffer str = ByteBuffer.allocate ( fragmentSize + 1);
                channel.read ( str, offset - 1);
                out = Helper.checkBeginningOfChunk ( new String ( str.array () ) );
            }
        } catch (Exception e) {
            e.printStackTrace ();
        }
        return out;
    }
}


