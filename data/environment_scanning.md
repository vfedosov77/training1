Best Practices for Preparing and Scanning an Environment


Area Targets are a target type that can be used to track parts of your surroundings. It offers unique opportunities for augmenting any part of the environment that was present during scanning. In order to use Area Targets, special hardware is required for scanning the chosen environment and each scanning device has its own guidelines. 

This article informs of the practices for choosing, preparing, and capturing an environment. The article is divided into sections that describe practices for supported scanning equipment such as professional 3D scanners or handheld scanning devices using LiDAR. For a list of devices capable of creating scans suitable for Area Targets, please see the Area Targets Overview page.

Scanning equipment can be grouped into

professional scanners are high-performance devices capable of covering large areas returning highly accurate 3D-models as colored point clouds and high-quality HDR panoramic images. See our Leica and NavVis documentation for more information.
A special class of these scanners is best suited for scanning medium sized spaces with a good accuracy and panorama images, such as the Matterport™ cameras. See our Matterport™ documentation for more information.
handheld scanners can capture room-sized spaces producing a 3D model well suited for tracking and authoring. The scan quality is fair, but less accurate and with fewer details compared to professional scanners. In addition, the scanning duration is limited to short capture periods putting a limit on the size of the space captured. The Vuforia Creator App is an integrated solution to provide direct, on-device Area Target generation.
Please follow the corresponding section of this page on best practices based on your scanner or scanning solution.

Choosing Environments
The space should be static; objects that are included in the scan should be fixed and unlikely to be moved. In environments, such as exhibition booths or office spaces with many movable elements or persons, verify that enough portions of the surroundings – such as wall decorations, ceiling-based installations, floor covering, stable furniture or similar are always visible during the intended augmentation scenarios.

Scanner Selection Guidelines
Exhibition booths, small cafés, apartments, production lines, factory floors, shops, museums, and even airports are all suitable candidates to scan and produce an Area Target from. As diverse these environments are as different are the solutions and equipment that exists to scan them in 3D. Due to differences in the layout and complexity of the spaces, as well as their visual appearance (cluttered vs. empty; sunlit vs. windowless, …), choosing an appropriate scanning method can be difficult. To select the most suitable scanning device, follow our recommendations based on the size of the space to be covered:

10 m2 up to 50 m2 or 500 sq ft are best captured with the Vuforia Creator App or Vuforia Area Target Capture API.
100 m2 up to 1000 m2 or 10,000 sq ft and medium spaces are best captured with the Matterport Pro2 & Pro3 scanners and the Leica scanners.
Spaces up to and beyond 30.000 m² or roughly 300,000 sq ft are of considerable size and best captured with NavVis scanners and the Leica RTC360. Such large spaces are in practice sliced into separate scans and processed individually for convenient authoring and better performance.
NOTE: Large Area Targets are represented by larger datasets which require the usage of the setExternalPosition() API to help localizing the user in the vast area. See Area Targets API Overview for more information.

Preparing the space
Inspect the space, tidy up the environment, and remove items that you do not wish to be included in the scan.
In indoor spaces, minimize incoming incident sunlight from windows. Scans on overcast days will not create hard shadows and provide average lighting: not too dark, not too bright. At the same time, ensure enough consistent artificial lighting is provided, by e.g. switching on lamps and lights, providing a typical lighting scenario.
Open doors within your space to allow for good registration between scan locations.
Close the doors to rooms and spaces that are not to be included.
Outdoor scans are a special circumstance which not all depth scanners support. An outdoor environment is further influenced by changing daylight, seasons, shadows, and growing vegetation, which makes it difficult to construct a reliable Area Target that tracks well - However, we have seen customers succeedingperfectly with augmented experiences outdoors.
Scanning Practices for Professional Scanners


Planning your path and scan positions

Before starting to scan, you should follow a normal walking path through the space. Mark the scanning positions with tape in a grid-like pattern with up to 3m (10ft) apart and 0.6 m (2ft) from walls, doors, and objects. Remember to ensure a clean line of sight between each scan position and to capture the entire space without distortions, slices, or warps.

Make sure that natural points of interest (i.e., at a workstation, desk, or machinery) are within the marked positions.
In large open indoor spaces, the scanning positions can be further apart if these areas are not of interest or a part of the user experience.
Avoid marking scanning positions in corners or too close to walls.
In cases where you move around a corner, mark down also at the corner’s edge.
For doors, there are different guidelines depending on the scanning device:
Matterport™ scanner should scan before and after entering a new room while maintaining the above-mentioned distance.
Leica scanner should scan in doorways to link to the previous scan position.
In both cases it is recommended to keep doors within the space open, while doors that let you leave the place should be shut.

Make additional scans to capture more detail where a visitor or user would look for more information.


Above illustration shows an example of the scanning path for the Matterport™ Pro2 or Pro3 camera.

During scanning
Avoid scanning while there is activity in the space. Similarly, you should also stay clear of being present in the scans.

If possible, monitor the scanning progress via a provided application on a tablet to ensure correct alignment of the new parts in-between scans. Correct failed scans by re-scanning on the position or move closer to the prior scan for achieving a better result that is compatible with your model.

Each professional scanner has their own scanning practices which we encourage you to study beforehand. For each brand, we have summarized a few in their detailed guides:

Leica – Scanning an Environment
Matterport™ - Scanning an Environment
NavVis – Preparing the Environment
Completing the scan
Completing the scan depends on the scanning device and the associated post-processing software. The raw scan data is likely transferred from the device to a processing software where the scanned data are registered together into a consistent 3D model of the environment. 

Matterport™ scans are uploaded to the cloud for processing via the Matterport™ Capture App. See Preparing the Camera and the Environment for additional details.
Use the NavVis IVION platform to process raw NavVis scans. See our Area Targets from NavVis Scans guide for details.
Use the Cyclone REGISTER 360 PLUS desktop tool for Leica scans. See Transferring and Preparing the Scan for details.
In general, review the new digital model for any missing parts or inconsistencies such as misalignments and broken links. Cleaning and trimming the scan may also be increasingly challenging if the scan was captured outside where distant objects in e.g., a long street got included.

Scanning Practices for Handheld Scanners


Planning your path and scanning motion

With smaller spaces it is also advisable to plan a path. The distance to surfaces and objects should be scanned at a distance between 0.5 m (2 ft) to 2 m (6.5 ft). Scan the area fronto-parallel, which means only scanning surfaces and objects directly in front of you.

Prepare a path along the walls or boundaries that will allow you to scan all the major features in the room. Scanning the same area multiple times may introduce surface duplication in the reconstruction. Therefore, choose a start and end position near each other that also prevents you from rescanning your start position. 
The path should be laid out so, if possible, you only scan an area once.
Tables and flat structures such as screens do not require to be scanned from both sides. 
Similarly, scanning beneath, behind or around objects that are considered inaccessible, and unlikely for users to approach, is better to exclude than to include.
Complex objects such as vegetation are difficult to capture in detail.
Transparent or reflective surfaces are captured poorly by handheld scanners and it is recommended to avoid scanning them or covering them up.
NOTE: Scanning empty rooms provide insufficient features for Vuforia to track.

If you are scanning multiple spaces with the Area Target Capture API and plan to align them to a common origin, you should adjust your scanning path to overlap slightly between the scans as this helps compute the alignment between two targets. But you should still make sure that your scanned space is visually different to the area that you align with. See information about aligning Area Targets in the Area Target Capture API for Unity and for Native.



Illustration of a scanning path without loop closure.

During Scanning
Depending on the device there may be varying directions on how to best achieve a successful scan. We have found that moving at a steady speed and scanning in an up-and-downwards continual motion provides good results. The Vuforia Creator app overlays a live mesh that offers a helpful indication of the captured parts of the environment. 

Scan aligned (fronto-parallel) to the surfaces (walls and other facades) in your room and scan an area only once.
Scan in vertical motions (up and down).


Illustration of a fronto-parallel and continuous scanning motion in front of a wall.

Avoid too fast motions and rotations, it is likely to break the scanning process or result in a warped reconstruction. 
Maintain a scanning distance of 0.5 - 2 m (1.5 – 6.5 ft).
Avoid scanning an area that you are approaching from afar (more than 2 meters), and instead, enter that area within the recommended range of distance.
Completing the scan
The Vuforia Creator App has a time limit and will automatically conclude the scan if that time is reached. Furthermore, the app can generate an Area Target directly from the scan and let you test it right after. You can then investigate if the scanned space is tracking well all within one session. See Vuforia Creator App for a complete guide.

Assisting Area Target Tracking
Rooms and environments with more unique features and objects tend to do better in tracking and re-localization. In environments containing repeated structure and/or multiple instances of the same objects, tracking locations may be mistaken for one another or not be recognized. To accommodate spaces with less features or repetitions, we recommend adding additional features such as prominent objects or markers that can aid the tracking, or you may use the location prior for (first-time) localization. See the Area Target API Overview for more information.

Reflective and transparent surfaces are commonly difficult to capture. If the environment possesses glass walls, glass tables, a large surface with a glossy metal with reflections, consider covering such surfaces or just not scan them. 
