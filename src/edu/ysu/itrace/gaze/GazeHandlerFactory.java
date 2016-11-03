package edu.ysu.itrace.gaze;

import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.ui.IWorkbenchPartReference;
import org.eclipse.draw2d.FigureCanvas;
 
import edu.ysu.itrace.gaze.handlers.ClassMLGazeHandler;
import edu.ysu.itrace.gaze.handlers.BrowserGazeHandler;
import edu.ysu.itrace.gaze.handlers.StyledTextGazeHandler;

/**
 * Creates IGazeHandlers from objects within the Workbench.
 */
public class GazeHandlerFactory {

    /**
     * Creates and returns a new IGazeHandler object from the specified object,
     * or returns null if no handler object is defined for that object.
     */
	 public static IGazeHandler createHandler(Object target, IWorkbenchPartReference partRef) {
        // create gaze handler for a StyledText widget
        if (target instanceof StyledText) {
            return new StyledTextGazeHandler(target); 
        }
        
        if (target instanceof Browser) {
        //create gaze handler for a Browser Stack overflow and Bug Report widget
        	return new BrowserGazeHandler(target);
        }

                // create gaze handler for a FigureCanvas
        if (target instanceof FigureCanvas) {
        	return new ClassMLGazeHandler(target, partRef);
        
        }
        return null;
    }
    
}
