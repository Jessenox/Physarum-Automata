package ServerDiscontinued;
import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.*;
import java.util.ArrayList;
import java.util.List;

public class LidarClientUDP extends JFrame {
    private List<Point> lidarPoints = new ArrayList<>();

    public LidarClientUDP() {
        setTitle("LiDAR Visualization");
        setSize(600, 600);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        // Panel donde se dibujarán los puntos del LiDAR
        JPanel drawPanel = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                g.setColor(Color.RED);

                synchronized (lidarPoints) {
                    for (Point point : lidarPoints) {
                        g.fillOval(point.x, point.y, 5, 5);
                    }
                }
            }
        };

        add(drawPanel);

        // Hilo para recibir los datos UDP
        new Thread(() -> connectToServer(drawPanel)).start();
    }

    // Conexión al servidor UDP para recibir los datos binarios
    private void connectToServer(JPanel drawPanel) {
        try (DatagramSocket socket = new DatagramSocket()) {
            byte[] buffer = new byte[8]; // Para recibir dos floats

            DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
            socket.connect(InetAddress.getByName("127.0.0.1"), 65432);  // Conéctate al servidor

            while (true) {
                socket.receive(packet);

                ByteArrayInputStream bis = new ByteArrayInputStream(buffer);
                DataInputStream dis = new DataInputStream(bis);

                float x = dis.readFloat();
                float y = dis.readFloat();

                System.out.println("Datos recibidos: (" + x + ", " + y + ")");

                synchronized (lidarPoints) {
                    lidarPoints.add(new Point((int) x + getWidth() / 2, getHeight() / 2 - (int) y));  // Ajusta el centro
                }

                drawPanel.repaint();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            LidarClientUDP client = new LidarClientUDP();
            client.setVisible(true);
        });
    }
}
