import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

public class XargsEx {

	private static int num = -1; // -n command
	private static String replace = null; // -I command
	private static boolean printCommand = false; //-t command
	private static boolean noRunIfEmpty = false; //-r command
	private static List<String> commandArguments = new ArrayList<>(); //stores the command arguments

    public static void main(String[] args) {
    	if (args.length < 1) {
    		printUsage();
    		return;
    	}
    	parser(args);
    	
    	List<String> inputs = readInputs();
    	if(inputs.isEmpty() && noRunIfEmpty) {
    		return;
    	}
    	if(replace != null) {
    		withReplace(inputs);
    	}
    	else {
    		withoutReplace(inputs);
    	}
    }
    
    private static void printUsage() {
    	System.out.println("Usage: java Xargs.java [-n num] [-I replace] [-t] [r] commandArguments");
    }

    private static void parser(String[] arguments) {
    	for(int i = 0; i < arguments.length; i++) {
    		switch(arguments[i]) {
	    		case "-n":
	    			if(i + 1 < arguments.length) {
	    				try {
	    					num = Integer.parseInt(arguments[++i]);
	    				}
	    				catch(NumberFormatException e) {
	    					printUsage();
	    					return;
	    				}
	    			}
	    			else {
	    				printUsage();
	    				return;
	    			}
	    			break;   
	    			
	    		case "-I":
	    			if(i + 1 < arguments.length) {
	    				replace = arguments[++i];
	    			}
	    			else {
	    				printUsage();
	    				return;
	    			}
	    			break;
	    		case "-t":
	    			printCommand = true;
	    			break;
	    		case "-r":
	    			noRunIfEmpty = true;
	    			break;
	    		default:
	    			if(arguments[i].startsWith("-")) {
	    				printUsage();
	    				return;
	    			}
	    			else {
	    				commandArguments.add(arguments[i]);
	    		}
	    	}
	    }
    }
    
    private static List<String> readInputs() {
    	List<String> inputs = new ArrayList<>();
    	try(BufferedReader reader = new BufferedReader(new InputStreamReader(System.in))) {
    		String line;
    		while((line = reader.readLine()) != null) {
    			for(String word: line.trim().split("\\s+")) {
    				inputs.add(sanitizeInput(word));
    			}
    		}
    	}
    	catch(IOException e) {
    		e.printStackTrace();
    	}
    	return inputs;
    }
    
    private static String sanitizeInput(String input) {
    	return input.replaceAll("[;&|><*?()$]", "");
    }
    
    private static void withReplace(List<String> inputs) {
    	for(List<String> batch : getBatches(inputs, num)) {
    		List<String> finalCommand = replacePlaceholders(commandArguments, batch);
    		if(printCommand) {
    			System.out.println(String.join(" ", finalCommand));
    		}
    		executeCommand(finalCommand);
    	}
    }
    
    private static void withoutReplace(List<String> inputs) {
    	for(List<String> batch : getBatches(inputs, num)) {
    		List<String> finalCommand = new ArrayList<>(commandArguments);
    		finalCommand.addAll(batch);
    		if(printCommand) {
    			System.out.println(String.join(" ",  finalCommand));
    		}
    		executeCommand(finalCommand);
    	}
    }
    
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
    
    private static void executeCommand(List<String> commandArguments) {
    	try {
    		ProcessBuilder pb = new ProcessBuilder(commandArguments); //process builder is initialized with the commands and arguments
    		pb.inheritIO(); //new processes inherit standard input and output
    		Process process = pb.start(); // process start, returns a process object
    		process.waitFor(); //waits until process completes
    	}
    	catch(IOException | InterruptedException e) {
    		e.printStackTrace();
    	}
    }
}
