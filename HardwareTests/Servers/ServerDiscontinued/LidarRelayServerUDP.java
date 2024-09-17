package ServerDiscontinued;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;

public class LidarRelayServerUDP {
    private static final int LIDAR_PORT = 8080;    // Port for the LiDAR client
    private static final int CLIENT_PORT = 8081;   // Port for visualization clients
    private static final int BUFFER_SIZE = 1024;   // Size of the UDP packet buffer
    private static List<InetAddress> clientAddresses = new ArrayList<>();

    public static void main(String[] args) {
        new Thread(LidarRelayServerUDP::receiveFromLidarClient).start();
        new Thread(LidarRelayServerUDP::listenForClientRegistrations).start();
    }

    // Method to listen for visualization clients registering with the server
    private static void listenForClientRegistrations() {
        try (DatagramSocket serverSocket = new DatagramSocket(CLIENT_PORT)) {
            System.out.println("Listening for visualization client registrations on port " + CLIENT_PORT);

            byte[] buffer = new byte[BUFFER_SIZE];
            while (true) {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                serverSocket.receive(packet);

                InetAddress clientAddress = packet.getAddress();
                synchronized (clientAddresses) {
                    if (!clientAddresses.contains(clientAddress)) {
                        clientAddresses.add(clientAddress);
                        System.out.println("New visualization client registered: " + clientAddress);
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Method to receive LiDAR data from the LiDAR client and forward it to all visualization clients
    private static void receiveFromLidarClient() {
        try (DatagramSocket socket = new DatagramSocket(LIDAR_PORT)) {
            byte[] buffer = new byte[BUFFER_SIZE];
            System.out.println("Listening for LiDAR data on port " + LIDAR_PORT);

            while (true) {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                socket.receive(packet);

                String data = new String(packet.getData(), 0, packet.getLength());
                forwardDataToClients(data);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // Method to forward data to all registered visualization clients
    private static void forwardDataToClients(String data) {
        synchronized (clientAddresses) {
            for (InetAddress clientAddress : clientAddresses) {
                try (DatagramSocket socket = new DatagramSocket()) {
                    byte[] buffer = data.getBytes();
                    DatagramPacket packet = new DatagramPacket(buffer, buffer.length, clientAddress, CLIENT_PORT);
                    socket.send(packet);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
