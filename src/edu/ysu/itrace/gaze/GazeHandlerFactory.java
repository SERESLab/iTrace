package edu.ysu.itrace.gaze;

import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.ui.IEditorReference;
import org.eclipse.ui.IViewReference;
import org.eclipse.ui.IWorkbenchPartReference;

import edu.ysu.itrace.gaze.handlers.StyledTextGazeHandler;
import edu.ysu.itrace.gaze.handlers.ProjectExplorerGazeHandler;

/**
 * Creates IGazeHandlers from objects within a workbench part.
 */
public class GazeHandlerFactory {

    /**
     * Creates and returns a new IGazeHandler object from the specified object
     * and partRef, or returns null if no handler object is defined for that object.
     */
    public static IGazeHandler createHandler(Object target,
            IWorkbenchPartReference partRef) {
        // create gaze handler for a StyledText widget within an EditorPart
        if (target instanceof StyledText &&
                partRef instanceof IEditorReference) {
            return new StyledTextGazeHandler(target, partRef);
        } else if (target instanceof Tree &&
        		partRef instanceof IViewReference) {
            return new ProjectExplorerGazeHandler(target, partRef);
        }

        return null;
    }
}
