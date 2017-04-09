# mrv2ply
Converter of .mrv format file (molecular descriptor based on xml) to .ply format (3d mesh).

```
$ ./mrv2ply 
Usage: ./mrv2ply <MRV_FILE> <EDGE_THICKNESS> <EDGE_SUBDIVISION> <SPHERE_RADIUS> <SPHERE_SUBDIVISION_LONGITUDE> <SPHERE_SUBDIVISION_LATITUDE> <HYDRONGENE_RADIUS_SCALE> <PLY_MESH_OUTPUT>
   ex: ./mrv2ply 1.mrv 0.1 20 0.3 20 20 1 molecule.ply
   ex: ./mrv2ply 1.mrv 0.1 20 0.4 20 20 0.5 molecule.ply
   
$ ./mrv2ply 1.mrv 0.1 20 0.3 20 20 1 molecule.ply
$ your_3d_viewer molecule.ply
```
