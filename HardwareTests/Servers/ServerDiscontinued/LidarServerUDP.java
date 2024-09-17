package ServerDiscontinued;
import java.net.*;
import java.io.*;

public class LidarServerUDP {
    private static final int UDP_PORT = 65432;

    public static void main(String[] args) {
        try (DatagramSocket udpSocket = new DatagramSocket(UDP_PORT)) {
            byte[] buffer = new byte[8]; // Para dos floats (4 bytes cada uno)

            System.out.println("Servidor UDP escuchando en el puerto " + UDP_PORT);

            while (true) {
                DatagramPacket packet = new DatagramPacket(buffer, buffer.length);
                udpSocket.receive(packet);

                // Convierte los datos binarios recibidos en floats (big-endian)
                ByteArrayInputStream bis = new ByteArrayInputStream(buffer);
                DataInputStream dis = new DataInputStream(bis);
                float x = dis.readFloat();
                float y = dis.readFloat();

                System.out.println("Datos recibidos: (" + x + ", " + y + ")");
            }
        } catch (IOException e) {
            System.err.println("Error en el servidor UDP: " + e.getMessage());
        }
    }
}
