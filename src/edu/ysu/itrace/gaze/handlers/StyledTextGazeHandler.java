package edu.ysu.itrace.gaze.handlers;

import java.util.Hashtable;
import java.util.Map;

import org.eclipse.jface.text.ITextOperationTarget;
import org.eclipse.jface.text.ITextViewer;
import org.eclipse.jface.text.JFaceTextUtil;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.graphics.Point;
import org.eclipse.ui.IEditorPart;
import org.eclipse.ui.IEditorReference;
import org.eclipse.ui.IWorkbenchPartReference;
import org.eclipse.ui.texteditor.ITextEditor;

import edu.ysu.itrace.AstManager;
import edu.ysu.itrace.ControlView;
import edu.ysu.itrace.Gaze;
import edu.ysu.itrace.gaze.IGazeHandler;
import edu.ysu.itrace.gaze.IGazeResponse;


/**
 * Implements the gaze handler interface for a StyledText widget.
 */
public class StyledTextGazeHandler implements IGazeHandler {
    private IWorkbenchPartReference partRef;
    private StyledText targetStyledText;
    private ITextViewer viewer;

    /**
     * Constructs a new gaze handler for the target StyledText object within
     * the workbench part specified by partRef.
     */
    public StyledTextGazeHandler(Object target,
                                 IWorkbenchPartReference partRef) {
        assert(target instanceof StyledText);
        this.targetStyledText = (StyledText)target;
        this.partRef = partRef;
        if (partRef instanceof IEditorReference) {
	        IEditorPart part = ((IEditorReference) partRef).getEditor(false);
			if (part instanceof ITextEditor) {
				ITextOperationTarget t = (ITextOperationTarget)part.getAdapter(ITextOperationTarget.class);
			    if (t instanceof ITextViewer) {
			        viewer = (ITextViewer)t;
			    }
			}
        }
    }

    @Override
    public IGazeResponse handleGaze(final int absoluteX, final int absoluteY,
            final int relativeX, final int relativeY, final Gaze gaze) {
        final AstManager astManager =
                (AstManager) targetStyledText.getData(ControlView.KEY_AST);

        IGazeResponse response = new IGazeResponse() {

            private String name = partRef.getPartName();
            private String type = null;
            private Map<String,String> properties
                    = new Hashtable<String,String>();

            // construct the type and properties for the response
            {
                try {
                    //Line and font height.
                    this.properties.put("line_height",
                                        String.valueOf(getLineHeight()));
                    this.properties.put("font_height",
                                        String.valueOf(getFontHeight()));

                    int lineIndex = targetStyledText.getLineIndex(relativeY);
                    int originalLineIndex = JFaceTextUtil.widgetLine2ModelLine(viewer, lineIndex);
                    
                    int lineOffset
                            = targetStyledText.getOffsetAtLine(lineIndex);
                    int offset = targetStyledText.getOffsetAtLocation(
                            new Point(relativeX, relativeY));
                    int col = offset - lineOffset;

                    this.properties.put("line", String.valueOf(lineIndex + 1));
                    this.properties.put("col", String.valueOf(col));

                    //(0, 0) relative to the control in absolute screen
                    //coordinates.
                    Point relativeRoot = new Point(absoluteX - relativeX,
                            absoluteY - relativeY);

                    //Top-left position of the first character on the line in
                    //relative coordinates.
                    Point lineAnchorPosition = targetStyledText.
                            getLocationAtOffset(targetStyledText.
                            getOffsetAtLine(lineIndex));
                    //To absolute.
                    lineAnchorPosition = new Point(
                            lineAnchorPosition.x + relativeRoot.x,
                            lineAnchorPosition.y + relativeRoot.y);
                    //Write out the position at the top-left of the first
                    //character in absolute screen coordinates.
                    this.properties.put("line_base_x",
                                        String.valueOf(lineAnchorPosition.x));
                    this.properties.put("line_base_y",
                                        String.valueOf(lineAnchorPosition.y));

                    AstManager.SourceCodeEntity[] entities =
                            astManager.getSCEs(originalLineIndex + 1, col);
                    String names = "";
                    String types = "";
                    String hows = "";
                    String signatures = "";
                    String declarations = "";
                    for (AstManager.SourceCodeEntity entity : entities) {
                        names += entity.name != null ? (entity.name + ";") :
                                                       ";";
                        types += entity.type != null ?
                                (entity.type.name() + ";") : ";";
                        hows += entity.how != null ? entity.how.name() + ";" :
                                ";";
                        signatures += entity.signature + ";";
                        declarations += entity.declaration + ";";
                        signatures += entity.signature != null ?
                                      (entity.signature + ";") : ";";
                        declarations += entity.declaration != null ?
                                        (entity.declaration + ";") : ";";
                    }
                    this.properties.put("fullyQualifiedNames",
                            names.length() > 0 ?
                            names.substring(0, names.length() - 1) : "");
                    this.properties.put("types", types.length() > 0 ?
                            types.substring(0, types.length() - 1) : "");
                    this.properties.put("hows", hows.length() > 0 ?
                            hows.substring(0, hows.length() - 1) : "");
                    this.properties.put("signature", signatures.length() > 0 ?
                            signatures.substring(0, signatures.length() - 1) :
                            "");
                    this.properties.put("declaration",
                            declarations.length() > 0 ? declarations.
                            substring(0, declarations.length() - 1) : "");
                } catch(IllegalArgumentException e) { }

                this.type = "text";
            }

            @Override
            public String getName() {
                return this.name;
            }

            @Override
            public String getType() {
                return this.type;
            }

            @Override
            public Map<String, String> getProperties() {
                return this.properties;
            }

            @Override
            public Gaze getGaze() {
                return gaze;
            }

            public IGazeHandler getGazeHandler() {
            return StyledTextGazeHandler.this;
            }
        };

        return (response.getType() != null ? response : null);
    }

    public int getLineHeight() {
        return targetStyledText.getLineHeight();
    }

    public int getFontHeight() {
        return targetStyledText.getFont().getFontData()[0].getHeight();
    }
}
