package edu.ysu.itrace.solvers;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Date;
import java.util.Iterator;
import java.util.Map.Entry;
import java.text.SimpleDateFormat;

import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.UIManager;

import org.eclipse.core.resources.ResourcesPlugin;

import com.google.gson.stream.JsonWriter;

import edu.ysu.itrace.gaze.IGazeResponse;

/**
 * Solver that simply dumps gaze data to disk in JSON format.
 */
public class JSONGazeExportSolver implements IFileExportSolver {
    private JsonWriter responseWriter;
    private File outFile;
    private String filenamePattern =
            "'gaze-responses-'yyyyMMdd'T'HHmmss','SSSSZ'.json'";
    private Dimension screenRect;

    public JSONGazeExportSolver() {
    }

    @Override
    public String getFilenamePattern() {
        return filenamePattern;
    }

    @Override
    public void setFilenamePattern(String filenamePattern) {
        this.filenamePattern = filenamePattern;
    }

    @Override
    public void init() {
        screenRect = Toolkit.getDefaultToolkit().getScreenSize();
        try {
            outFile = new File(getFilename());

            // Check that file does not already exist. If it does, do not begin
            // tracking.
            if (outFile.exists()) {
                System.out.println(friendlyName());
                System.out.println("You cannot overwrite this file. If you "
                        + "wish to continue, delete the file " + "manually.");

                return;
            }

            responseWriter = new JsonWriter(new FileWriter(outFile));
            responseWriter.setIndent("  ");
        } catch (IOException e) {
            throw new RuntimeException("Log files could not be created: "
                    + e.getMessage());
        }
        System.out.println("Putting files at " + outFile.getAbsolutePath());

        try {
            responseWriter.beginObject()
                              .name("environment")
                              .beginObject()
                                  .name("screen_size")
                                  .beginObject()
                                      .name("width")
                                      .value(screenRect.width)
                                      .name("height")
                                      .value(screenRect.height)
                                  .endObject()
                              .endObject()
                              .name("gazes")
                              .beginArray();
        } catch (IOException e) {
            throw new RuntimeException("Log file header could not be written: "
                    + e.getMessage());
        }
    }

    @Override
    public void process(IGazeResponse response) {
        try {
            if (response.getProperties().size() > 0) {
                int screenX =
                        (int) (screenRect.width * response.getGaze().getX());
                int screenY =
                        (int) (screenRect.height * response.getGaze().getY());

                responseWriter.beginObject()
                                .name("file")
                                .value(response.getName())
                                .name("type")
                                .value(response.getType())
                                .name("x")
                                .value(screenX)
                                .name("y")
                                .value(screenY)
                                .name("left_validation")
                                .value(response.getGaze().getLeftValidity())
                                .name("right_validation")
                                .value(response.getGaze().getRightValidity())
                                .name("left-pupil-diameter")
                                .value(response.getGaze().getLeftPupilDiameter())
                                .name("right-pupil-diameter")
                                .value(response.getGaze().getRightPupilDiameter())
                                .name("tracker_time")
                                .value(response.getGaze().getTrackerTime().getTime())
                                .name("system_time")
                                .value(response.getGaze().getSystemTime())
                                .name("nano_time")
                                .value(response.getGaze().getNanoTime());

                for (Iterator<Entry<String, String>> entries =
                        response.getProperties().entrySet().iterator(); entries
                        .hasNext();) {
                    Entry<String, String> pair = entries.next();
                    responseWriter.name(pair.getKey())
                                  .value(pair.getValue());
                }

                responseWriter.endObject();
            }
        } catch (IOException e) {
            // ignore write errors
        }
    }

    @Override
    public void dispose() {
        try {
            responseWriter.endArray().endObject();
            responseWriter.flush();
            responseWriter.close();
            System.out.println("Gaze responses saved.");
        } catch (IOException e) {
            throw new RuntimeException("Log file footer could not be written: "
                    + e.getMessage());
        }
    }

    @Override
    public String getFilename() {
        String workspaceLocation =
                ResourcesPlugin.getWorkspace().getRoot().getLocation()
                        .toString();
        try {
            SimpleDateFormat formatter = new SimpleDateFormat(filenamePattern);
            return workspaceLocation + "/" + formatter.format(new Date());
        } catch (IllegalArgumentException e) {
            return workspaceLocation + "/" + filenamePattern;
        }
    }

    @Override
    public String friendlyName() {
        return "JSON Gaze Export";
    }

    @Override
    public void config() {
    	UIManager.put("swing.boldMetal", new Boolean(false)); //make font plain
    	
    	JTextField configVal = new JTextField(getFilenamePattern());
    	
    	JPanel configPanel = new JPanel();
    	configPanel.setLayout(new BoxLayout(configPanel, BoxLayout.Y_AXIS)); //vertically align
        configPanel.add(new JLabel("Export Filename Pattern"));
        configPanel.add(configVal);
        configPanel.setPreferredSize(new Dimension(300,40)); //resize appropriately

        final int configDialog = JOptionPane.showConfirmDialog(null, configPanel, 
                 friendlyName() + " Configuration", JOptionPane.OK_CANCEL_OPTION,
                 JOptionPane.PLAIN_MESSAGE);
        if (configDialog == JOptionPane.OK_OPTION) {
           setFilenamePattern(configVal.getText());
        }
    }
}
