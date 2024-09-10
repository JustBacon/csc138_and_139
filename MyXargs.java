import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;


// -n -I cant work together
public class MyXargs{
    public static void main(String[] args){
        if(args.length < 1){
            System.out.println("Usage: MyXargs [-n num] [-I replace] [-t] [-r] command");
        }
        if(args.length > 0){
            // ProcessBuilder pb = new ProcessBuilder(args[0]);
            // pb.inheritIO();
            // try {
            //     Process process = pb.start();
            //     process.waitFor();
            // } catch (IOException | InterruptedException e) {
            //     e.printStackTrace();
            // }
            // args.length - 1 will be the command
            System.out.println(args.length);
            // for(int i = 0; i < args.length; i++){
            //     System.out.println(args[i]);
            // }
        }
    }
}