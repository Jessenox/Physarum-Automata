package ServerDiscontinued;
import javax.swing.*;
import java.awt.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.util.ArrayList;
import java.util.List;

public class LidarSwingClientUDP extends JFrame {
    private final List<Point> lidarPoints = new ArrayList<>();
    private static final int MAP_SIZE = 800; // Size of the drawing panel
    private static final int SCALE_FACTOR = 100; // Scale to convert coordinates to pixels
    private static final int PORT = 8081; // Port to receive UDP packets

    public LidarSwingClientUDP() {
        setTitle("LiDAR Data Visualization");
        setSize(MAP_SIZE, MAP_SIZE);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        // Panel for drawing the LiDAR points
        JPanel drawingPanel = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                drawMap(g);
                drawLidarData(g);
            }
        };

        add(drawingPanel);
        new Thread(this::receiveData).start(); // Start the UDP listener in a new thread
    }

    // Method to draw the map grid and the LiDAR center point
    private void drawMap(Graphics g) {
        g.setColor(Color.LIGHT_GRAY);

        // Draw grid lines
        int center = MAP_SIZE / 2;
        for (int i = 0; i < MAP_SIZE; i += 50) {
            g.drawLine(i, 0, i, MAP_SIZE); // Vertical lines
            g.drawLine(0, i, MAP_SIZE, i); // Horizontal lines
        }

        // Draw the center point (LiDAR sensor)
        g.setColor(Color.BLUE);
        g.fillOval(center - 5, center - 5, 10, 10); // Draw a small circle to represent the LiDAR
    }

    // Method to draw LiDAR data points
    private void drawLidarData(Graphics g) {
        int center = MAP_SIZE / 2; // Center of the map
        synchronized (lidarPoints) {
            for (Point point : lidarPoints) {
                // Convert point coordinates to the panel's coordinate system
                int x = center + point.x;
                int y = center - point.y;

                // Set point color based on distance (closer points are red, farther points are green)
                float distance = (float) Math.sqrt(point.x * point.x + point.y * point.y) / SCALE_FACTOR;
                g.setColor(getPointColor(distance, 8.0f)); // Assuming max range is 8 meters

                g.fillOval(x, y, 5, 5); // Draw each point as a small circle
            }
        }
    }

    // Method to receive data over UDP
    private void receiveData() {
        try (DatagramSocket socket = new DatagramSocket(PORT)) {
            byte[] buffer = new byte[1024];

            while (true) {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                socket.receive(packet); // Receive the UDP packet

                String data = new String(packet.getData(), 0, packet.getLength());
                parseLidarData(data);
                SwingUtilities.invokeLater(this::repaint); // Request a repaint on the Event Dispatch Thread
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Method to parse the LiDAR data from the UDP packet
    private void parseLidarData(String data) {
        List<Point> newPoints = new ArrayList<>();
        String[] lines = data.split("\n");

        for (String line : lines) {
            try {
                // Parse the x and y coordinates from the input line
                if (line.startsWith("x:") && line.contains(", y:")) {
                    String[] parts = line.split(",");
                    float x = Float.parseFloat(parts[0].split(":")[1].trim());
                    float y = Float.parseFloat(parts[1].split(":")[1].trim());

                    // Convert to integer pixels and add to the list
                    newPoints.add(new Point((int) (x * SCALE_FACTOR), (int) (y * SCALE_FACTOR)));
                }
            } catch (Exception e) {
                System.out.println("Failed to parse line: " + line);
            }
        }

        // Update the lidarPoints list in a synchronized block
        synchronized (lidarPoints) {
            lidarPoints.clear();
            lidarPoints.addAll(newPoints);
        }
    }

    // Method to determine point color based on distance
    private Color getPointColor(float distance, float maxRange) {
        float ratio = distance / maxRange;
        int red = (int) (255 * (1 - ratio));
        int green = (int) (255 * ratio);
        return new Color(red, green, 0); // Color gradient from red to green
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            LidarSwingClientUDP client = new LidarSwingClientUDP();
            client.setVisible(true);
        });
    }
}
