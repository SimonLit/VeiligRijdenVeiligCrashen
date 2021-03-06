﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Threading;

namespace Communication_verzekering
{
    class AsynchronousSocketListener
    {
        // Thread signal.
        public static ManualResetEvent allDone = new ManualResetEvent(false);
        bool Data_received = false;
        public static string Data_Received_Export;

        public AsynchronousSocketListener()
        {
        }

        public static void StartListening()
        {
            // Data buffer for incoming data.
            byte[] bytes = new Byte[1024];

            // Establish the local endpoint for the socket.
            // The DNS name of the computer
            // running the listener is "host.contoso.com".
            //IPHostEntry ipHostInfo = Dns.Resolve(Dns.GetHostName());
            //IPAddress ipAddress = ipHostInfo.AddressList[0];
            //IPEndPoint localEndPoint = new IPEndPoint(ipAddress, 6000);

            IPEndPoint localEndPoint = new IPEndPoint(IPAddress.Parse("192.168.1.101"), 6000);

            // Create a TCP/IP socket.
            Socket listener = new Socket(AddressFamily.InterNetwork,
                SocketType.Stream, ProtocolType.Tcp);

            // Bind the socket to the local endpoint and listen for incoming connections.
            try
            {

                listener.Bind(localEndPoint);
                listener.Listen(100);

                while (true)
                {
                    // Set the event to nonsignaled state.
                    Console.WriteLine("connection");
                    allDone.Reset();

                    // Start an asynchronous socket to listen for connections.
                    //Console.WriteLine("Waiting for a connection...");
                    listener.BeginAccept(
                        new AsyncCallback(AcceptCallback),
                        listener);

                    // Wait until a connection is made before continuing.
                    allDone.WaitOne();
                }

            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }

            //Console.WriteLine("\nPress ENTER to continue...");
            //Console.Read();

        }

        public static void AcceptCallback(IAsyncResult ar)
        {
            // Signal the main thread to continue.
            allDone.Set();

            // Get the socket that handles the client request.
            Socket listener = (Socket)ar.AsyncState;
            Socket handler = listener.EndAccept(ar);

            // Create the state object.
            StateObject state = new StateObject();
            state.workSocket = handler;
            handler.BeginReceive(state.buffer, 0, StateObject.BufferSize, 0,
                new AsyncCallback(ReadCallback), state);
        }

        public static void ReadCallback(IAsyncResult ar)
        {
            String content = String.Empty;

            // Retrieve the state object and the handler socket
            // from the asynchronous state object.
            StateObject state = (StateObject)ar.AsyncState;
            Socket handler = state.workSocket;

            // Read data from the client socket. 
            int bytesRead = handler.EndReceive(ar);

            if (bytesRead > 0)
            {
                
                // There  might be more data, so store the data received so far.
                state.sb.Append(Encoding.ASCII.GetString(
                    state.buffer, 0, bytesRead));

                // Check for end-of-file tag. If it is not there, read 
                // more data.
                content = state.sb.ToString();
                Console.WriteLine(content);
                if (content.IndexOf('#') > -1)
                {

                    // All the data has been read from the 
                    // client. Display it on the console.
                    //Console.WriteLine("Read {0} bytes from socket. \n Data : {1}",
                    //    content.Length, content);
                    // Echo the data back to the client.
                    int count = 0;
                    foreach (char c in content)
                        if (c == '|') count++;
                    if (count >= 1)
                    {
                        if(content.Substring(11, 3) == "DAT")
                        {
                            DateTime datumstring = new DateTime();
                            datumstring = DateTime.Now;
                            content = content + "|" + datumstring.ToString();
                        }
                        Data_Received_Export = content;
                        int endingIndex = content.IndexOf("#");
                        content = content.Remove(0, endingIndex + 1);
                        int beginningIndex = content.IndexOf("@");
                        content = content.Substring(0, beginningIndex);
                        DateTime datum = new DateTime();
                        datum = DateTime.Now;
                        string filename = datum.ToString();
                        filename = filename + ".txt";
                        filename = filename.Replace(':', ',');
                        //filename = @"C:\Gebruiker\Documents\Mobile dash\" + filename + ".txt";
                        using (StreamWriter streamWriter = new StreamWriter(filename))
                        {
                            streamWriter.WriteLine(content);
                        }
                        Send(handler, "#ACK@");
                        bytesRead = 0;
                        content = "";
                        Array.Clear(state.buffer, 0, state.buffer.Length);
                        state.sb.Clear();
                        Console.WriteLine(content);
                        handler.BeginReceive(state.buffer, 0, StateObject.BufferSize, 0,
                        new AsyncCallback(ReadCallback), state);
                    }
                    else if (content == "#CONNECT@")
                    {
                        Send(handler, "#ACK@");
                        bytesRead = 0;
                        content = "";
                        Array.Clear(state.buffer, 0, state.buffer.Length);
                        state.sb.Clear();
                        Console.WriteLine(state.buffer);
                        handler.BeginReceive(state.buffer, 0, StateObject.BufferSize, 0,
                        new AsyncCallback(ReadCallback), state);
                    

                    }

                    else
                    {
                        Send(handler, "#Not ACK@");
                        Data_Received_Export = "";
                        bytesRead = 0;
                        content = "";
                        Array.Clear(state.buffer, 0, state.buffer.Length);
                        state.sb.Clear();
                        Console.WriteLine(state.buffer);
                        handler.BeginReceive(state.buffer, 0, StateObject.BufferSize, 0,
                        new AsyncCallback(ReadCallback), state);
                    }
                }

                else
                {
                    // Not all data received. Get more.
                    handler.BeginReceive(state.buffer, 0, StateObject.BufferSize, 0,
                    new AsyncCallback(ReadCallback), state);
                }
            }
        }

        public static void Export_data(IAsyncResult ar)
        {
            StateObject state = (StateObject)ar.AsyncState;
            string Data_Received_export = state.sb.ToString();
        }

        private static void Send(Socket handler, String data)
        {
            // Convert the string data to byte data using ASCII encoding.
            byte[] byteData = Encoding.ASCII.GetBytes(data);

            // Begin sending the data to the remote device.
            handler.BeginSend(byteData, 0, byteData.Length, 0,
                new AsyncCallback(SendCallback), handler);
        }

        private static void SendCallback(IAsyncResult ar)
        {
            //try
            //{
            // Retrieve the socket from the state object.
            Socket handler = (Socket)ar.AsyncState;

            // Complete sending the data to the remote device.
            int bytesSent = handler.EndSend(ar);
            //Console.WriteLine("Sent {0} bytes to client.", bytesSent);

            //handler.Shutdown(SocketShutdown.Both);
            //handler.Close();

            //}
            //catch (Exception e)
            //{
            //Console.WriteLine(e.ToString());
            //}
        }


        public static int MainSocketListen()
        {
            StartListening();
            return 0;

        }

    }
}
