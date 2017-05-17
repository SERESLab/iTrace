package edu.ysu.itrace.trackers;

import edu.ysu.itrace.*;
import edu.ysu.itrace.calibration.CalibrationStatusDisplay;
import edu.ysu.itrace.calibration.Calibrator;
import edu.ysu.itrace.exceptions.CalibrationException;

import java.awt.Dimension;
import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.Insets;
import java.awt.MouseInfo;
import java.awt.Point;
import java.awt.Toolkit;
import java.io.*;
import java.util.concurrent.LinkedBlockingQueue;

import javax.swing.JFrame;

import java.util.Date;

import org.eclipse.e4.core.services.events.IEventBroker;
import org.eclipse.ui.PlatformUI;
/**
 * Tracker which follows the mouse cursor. Useful for testing when no eye
 * tracker is present.
 */
public class SystemMouseTracker implements IEyeTracker {
    private static class TrackerThread extends Thread {
        private enum RunState {
            RUNNING,
            STOPPING,
            STOPPED
        }

        private SystemMouseTracker parent = null;
        private volatile RunState running = RunState.STOPPED;
        private IEventBroker eventBroker;
        
        public TrackerThread(SystemMouseTracker parent) {
            this.parent = parent;
            this.eventBroker = PlatformUI.getWorkbench().getService(IEventBroker.class);
        }

        public void startTracking() {
            start();
        }

        public void stopTracking() {
            //Already stopped.
            if (running != RunState.RUNNING)
                return;

            running = RunState.STOPPING;
            while (running != RunState.STOPPED) {
                try {
                    Thread.sleep(25);
                } catch (InterruptedException e) {
                    //Just try again.
                }
            }
        }

        //Executed on separate thread to get mouse "gaze" data.
        public void run() {
            running = RunState.RUNNING;
            while (running == RunState.RUNNING)
            {
                Point cursorPosition = MouseInfo.getPointerInfo().
                                       getLocation();
                Dimension screenSize = Toolkit.getDefaultToolkit().
                                       getScreenSize();
                double x = (double) cursorPosition.x /
                           (double) screenSize.width;
                double y = (double) cursorPosition.y /
                           (double) screenSize.height;
                Gaze gaze = new Gaze(x - 0.05, x + 0.05, y, y, 1.0, 1.0,
                                     0.0, 0.0, System.currentTimeMillis());
                parent.calibrator.moveCrosshair(cursorPosition.x,
                                                cursorPosition.y);
                parent.gazePoints.add(gaze);
                //if(Activator.getDefault().recording)
                	eventBroker.post("iTrace/newgaze", gaze);
                try {
                    Thread.sleep(25);
                } catch (InterruptedException e) {
                    //Just try again.
                }
            }
            running = RunState.STOPPED;
        }
    }

    private class SystemMouseCalibrator extends Calibrator {
        public SystemMouseCalibrator() throws IOException {
            super();
        }

        protected void startCalibration() throws Exception {
            //Do nothing.
        }

        protected void stopCalibration() throws Exception {
            //Do nothing.
        }

        protected void useCalibrationPoint(double x, double y)
                throws Exception {
            //Do nothing.
        }

		@Override
		protected void displayCalibrationStatus(JFrame frame) throws Exception {
        	CalibrationStatusDisplay calibDisplay = 
        			new CalibrationStatusDisplay(frame, calibrationPoints,new java.awt.geom.Point2D.Double[9]);
        	frame.setMinimumSize(new Dimension(600,300));
        	
        	frame.add(calibDisplay);
        	
        	frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        	frame.setExtendedState(JFrame.MAXIMIZED_BOTH);
        	frame.setVisible(true);
        	frame.setTitle("Calibration: "+new Date());
        	Insets insets = frame.getInsets();
        	int width = frame.getSize().width-(insets.left+insets.right);
        	int height = frame.getSize().height-(insets.top+insets.bottom);
        	calibDisplay.windowDimension = new Dimension(width,height);
        	frame.toFront();
        	calibDisplay.repaint();
		}
    }

    private LinkedBlockingQueue<Gaze> gazePoints
            = new LinkedBlockingQueue<Gaze>();
    private SystemMouseCalibrator calibrator;
    private TrackerThread trackerThread = null;

    public SystemMouseTracker() throws IOException {
        calibrator = new SystemMouseCalibrator();
    }

    public void close() {
        try {
            stopTracking();
        } catch (IOException e) {
            //There's not really much that can be done at this point.
        }
    }

    public void clear() {
        gazePoints = new LinkedBlockingQueue<Gaze>();
    }

    public void calibrate() throws CalibrationException {
        calibrator.calibrate();
        try {
        	//calibrator.displayCalibrationStatus();
        } catch (Exception e) {
        	e.printStackTrace();
        	throw new CalibrationException("Cannot display calibration status!");
        	
        }
    }

    public void startTracking() throws IOException {
        if (trackerThread != null)
            return;

        trackerThread = new TrackerThread(this);
        trackerThread.startTracking();
    }

    public void stopTracking() throws IOException {
        if (trackerThread == null)
            return;

        trackerThread.stopTracking();
        trackerThread = null;
    }

    public Gaze getGaze() {
        return gazePoints.poll();
    }

    public void displayCrosshair(boolean enabled) {
        calibrator.displayCrosshair(enabled);
    }

    public void setXDrift(int drift) {
    }

    public void setYDrift(int drift) {
    }
}
