import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

public class MyXargs{
    private static List<String> commandArgs = new ArrayList<>();
    private static int num = -1; // used for -n # command
    private static String replaceIcommand = null; // -I {} command
    private static boolean rCommand = false; // -r command
    private static boolean tCommand = false; // -t command
    public static void main(String[] args){
        // check the number of arguments used and how to use MyXargs
        if(args.length < 1){
            errorUsage();
            return;
        }

        // go through args list to see which commands are used
        for(int i = 0; i < args.length; i++){
            switch (args[i]) {
                case "-n":
                    if(i + 1 < args.length){
                        try {
                            num = Integer.parseInt(args[++i]);
                        } catch (NumberFormatException e) {
                            errorUsage();
                            return;
                        }
                    }
                    break;
                case "-I":
                    if(i + 1 < args.length){
                        replaceIcommand = args[++i];
                    }
                case "-r":
                    rCommand = true;
                    break;
                case "-t":
                    tCommand = true;
                    break;
                default:
                    if(args[i].startsWith("-")){
                        errorUsage();
                        return;
                    }else{
                        commandArgs.add(args[i]);
                    }
                    break;
            }
        }

        List<String> inputs = readInput();
        if(inputs.isEmpty() && rCommand) return;
        // user tried to use -n and -I together
        if(num != -1 && replaceIcommand != null){
            System.out.println("-n and -I are mutually exclusive");
            return;
        }

        // check if user used the -I command
        if(replaceIcommand != null){
            withICommand(inputs);
        }else{
            withoutICommand(inputs);
        }
    
    }

    private static void errorUsage(){
        System.out.println("Usage: MyXargs [-n num] [-I replace] [-t] [-r] command");
    }

    // grab the output from the left side
    private static List<String> readInput(){
        List<String> inputs = new ArrayList<>();
        try(BufferedReader reader = new BufferedReader(new InputStreamReader(System.in))) {
            String line;
            while((line = reader.readLine()) != null){
                for(String word: line.trim().split("\\s+")){
                    inputs.add(sanitizeInput(word));
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return inputs;
    }

    // remove this characters from the input ;&|><*?()$
    private static String sanitizeInput(String input){
        return input.replaceAll("[;&|><*?()$]", "");
    }

    // this method will run if -I {} is provided
    private static void withICommand(List<String> inputs) {
    	for(List<String> batch : getBatches(inputs, num)) {
    		List<String> finalCommand = replacePlaceholders(commandArgs, batch);
    		if(tCommand) {
    			System.out.println(String.join(" ", finalCommand));
    		}
    		runCommand(finalCommand);
    	}
    }

    // this method will run if there is no -I {}
    private static void withoutICommand(List<String> inputs){
        for(List<String> batch : getBatches(inputs, num)){
            List<String> finalCommand = new ArrayList<>(commandArgs);
    		finalCommand.addAll(batch);
    		if(tCommand) {
    			System.out.println(String.join(" ",  finalCommand));
    		}
            runCommand(finalCommand);
        }
    }

    // For -I {} command
    // replace {} with the string
    private static List<String> replacePlaceholders(List<String> commandArguments, List<String> replacements) {
    	List<String> replacedCommand = new ArrayList<>();
    	for(String part : commandArguments) {
    		if(part.contains("{}")) {
    			for(String replacement : replacements) {
    				replacedCommand.add(part.replace("{}", replacement));
    			}
    		}
    		else {
    			replacedCommand.add(part);
    		}
    	}
    	return replacedCommand;
    }

    // combinne the inputs
    private static List<List<String>> getBatches(List<String> inputs, int num) {
    	List<List<String>> batches = new ArrayList<>();
    	if(num > 0) {
    		for(int i = 0; i < inputs.size(); i += num) {
    			batches.add(inputs.subList(i, Math.min(i + num, inputs.size())));
    		}
    	}
    	else {
    		batches.add(inputs);
    	}
    	return batches;
    }

    // run processBuilder to run all the command with arguments
    private static void runCommand(List<String> commandArgs){
        ProcessBuilder pb = new ProcessBuilder(commandArgs);
        pb.inheritIO();
        try {
            Process process = pb.start();
            process.waitFor();
        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
        }
    }
}