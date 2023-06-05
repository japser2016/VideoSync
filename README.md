# VideoSync

VideoSync is a project that enables clients to watch videos together online, maintaining synchronization among all viewers. If one user pauses the video, it pauses for everyone else, allowing a collective watching experience even when viewers are in different locations.

## Introduction
This project is designed to offer a peer-to-peer (P2P) network solution to watch videos together with minimal latency. It aims to be an improvement over existing "watch together" programs by reducing communication between clients and thereby enhancing the watching experience.

## Architecture
VideoSync is divided into two main components: The Sync Server and the Web Server.

### Sync Server
The Sync Server manages the list of clients (other users) in the P2P network. It updates the list when new clients join and informs existing clients of the updated list. It also handles communication between clients regarding video states (Play, Pause, and current timestamp) and manages the initial video file transfer based on whether the client initiating the session already has the video file or not. After the video file transfer, the Sync Server starts the Web Server.

### Web Server
The Web Server controls the video playback on the front end. It keeps track of the video player's state and timestamp. Whenever the video is manually played or paused, these details are sent to the Sync Server to be propagated to other users. The Web Server continuously checks for updates from the Sync Server to synchronize video playback across all users.

## Features
* **P2P Network:** The P2P system allows for a robust network where everyone acts as a host, ensuring service continuity even if a host goes down.

* **File Sharing:** Users can request video files from others who have them, with a fail-safe mechanism to request from another user if the original one fails to send the file. File sharing uses the TCP protocol for reliable data transfer.

* **Low Latency UDP Communication:** VideoSync uses the UDP protocol to swiftly synchronize changes in video states (play, pause, current timestamp) across all users in the network.

* **Local Web-Based Video Player:** The video player is hosted on a local website built with Node.js, enabling automatic updates to play or pause the video based on network-wide commands.

## Implementation
To start a session, a user launches the Sync Server with basic information such as the listening port and the location of the video file. New users joining the network provide their listening port, the destination hostname to connect to, the port of the destination host, and a flag indicating whether they need the video file. The Sync Server performs a format check on the provided information.

On joining the network, new users receive a list of all other users in the network and send an "initial hello" message to everyone except the user they initially connected with. All users maintain a list of other users in the network, facilitating network-wide P2P communication.

If a new user needs the video file, they request it from other clients in the network, creating a TCP connection once the request is granted. After the video file transfer is complete, the Sync Server starts the local Web Server and the user can then connect to it via a web browser to watch the video.

Playback status changes (play/pause and timestamp) are communicated between the Web Server and Sync Server via text files, ensuring low latency. Changes are then propagated by the Sync Server to all other users in the network, keeping playback synchronized across all viewers.

The project primarily uses the UDP protocol for low-latency communication, following the "Stop and Wait" protocol. TCP is used only for the video file transfer to ensure reliability.


## Written by
gabman15
and
jasper2016
