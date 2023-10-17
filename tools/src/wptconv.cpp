#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

#define MAX_WAYPOINTS 1024
#define W_FILE_FL_READ_VISIBILITY (1<<0) // require to read visibility file

string toLowerCase(string str) {
    string out = str;

    for (int i = 0; str[i]; i++) {
        out[i] = tolower(str[i]);
    }

    return out;
}

bool fileExists(std::string path) {
    if (FILE* file = fopen(path.c_str(), "r"))
    {
        fclose(file);
        return true;
    }
    return false;
}

const char* g_HEXTOASCII = "0123456789ABCDEF";

class FileBuffer {
public:
    string data;
    int iIndex;
    int readPos = 0;
    
    FileBuffer(string s) {
        this->data = s;
    }

    uint8_t FromAscii(uint8_t n)
    {
        if (n >= 65)
            return (n - 55);

        return (n - 48);
    }

    uint32_t readByte()
    {
        if (readPos >= data.size()) {
            printf("ERROR: Read past EOF\n");
            return 0;
        }
        uint8_t nib0 = FromAscii(data[readPos++]);
        uint8_t nib1 = FromAscii(data[readPos++]);

        return ((nib1)+(nib0 << 4));
    }

    int32_t ReadInt32()
    {
        int32_t ret = 0;
        int32_t shift = 0;

        for (int32_t i = 0; i < 4; i++)
        {
            int32_t read = readByte();

            read <<= shift;

            shift += 8;

            ret += read;
        }

        return ret;
    }

    float ReadFloat()
    {
        uint32_t retInt = uint32_t(ReadInt32());
        return *((float*)&retInt);
    }
    
    string ReadString(int len)
    {
        string ret = "";

        while (len-- > 0)
        {
            uint8_t byte = readByte();

            if (byte == 0 && false)
                break;

            ret += (char)(byte);
        }

        while (len-- > 0)
        {
            readByte();
        }

        return ret;
    }
};

struct Vector {
    float x, y, z;
};

class CWaypoint {
public:
    vector<int> m_PathsTo;

    Vector m_vOrigin;
    // flags such as Jump, crouch etc
    int m_iFlags;

    bool Read ( FileBuffer& file, int index )
	{
		m_iFlags = file.ReadInt32();
		m_vOrigin.x = file.ReadFloat();		
		m_vOrigin.y = file.ReadFloat();
		m_vOrigin.z = file.ReadFloat();

		int numPaths = file.ReadInt32();

		m_PathsTo.clear();

		if ( numPaths < 32 )
		{
			for (int i = 0; i < numPaths; i ++ )
				m_PathsTo.push_back(file.ReadInt32());

			return true;
		}
		else
			return false;
		
	}
};

class CWaypointHeader {
public:
    // 8 characters
    string filetype;  // should be "RC_bot\0"
    int32_t waypoint_file_version = 0;
    int32_t waypoint_file_flags = 0;  // used for visualisation
    int32_t number_of_waypoints = 0;
    // 32 characters
    string mapname;  // name of map for these waypoints

    bool Read(FileBuffer& file)
    {
        filetype = file.ReadString(8);
        printf("FILETYPE = %s\n", filetype.c_str());
        waypoint_file_version = file.ReadInt32();
        printf("VERSION = %d\n", waypoint_file_version);
        waypoint_file_flags = file.ReadInt32();
        printf("FLAGS = %d\n", waypoint_file_flags);
        number_of_waypoints = file.ReadInt32();
        printf("NUM WPTS = %d\n", number_of_waypoints);

        mapname = file.ReadString(32);
        mapname = string(mapname.c_str()); // for accurate .size() when null char is <31 char position
        printf("MAPNAME = %s\n", mapname.c_str());

        return number_of_waypoints < MAX_WAYPOINTS;
    }
};

#pragma pack(push,1)
struct WAYPOINT_HDR {
    char filetype[8];  // should be "RCBot\0"
    int  waypoint_file_version;
    int  waypoint_file_flags;  // used for visualisation
    int  number_of_waypoints;
    char mapname[32];  // name of map for these waypoints
};

struct WAYPOINT {
    int  flags;		// button, lift, flag, health, ammo, etc.
    Vector origin;   // location  
};
#pragma pack(pop)

void convert_to_rcw(string rcwa_fpath) {
    string outpath = rcwa_fpath.substr(0, rcwa_fpath.length() - 1);
    string fname = rcwa_fpath.substr(0, rcwa_fpath.length() - 5);

    std::ifstream t(rcwa_fpath);
    std::stringstream buffer;
    buffer << t.rdbuf();
    string data = buffer.str();

    FileBuffer buf = FileBuffer(data);

    CWaypointHeader hdr;
    if (!hdr.Read(buf)) {
        printf("ERROR: Too many waypoints! (%d > %d)\n", hdr.number_of_waypoints, MAX_WAYPOINTS);
        return;
    }

    CWaypoint* input_wpts = new CWaypoint[hdr.number_of_waypoints];

    for (int i = 0; i < hdr.number_of_waypoints; i++)
    {
        if (!input_wpts[i].Read(buf, i))
        {
            printf("ERROR: waypoint %d is corrupt!\n", i);
            return;
        }
    }

    WAYPOINT_HDR outhdr;
    strncpy(outhdr.filetype, "RCBot", 8);
    outhdr.waypoint_file_version = 4;
    outhdr.waypoint_file_flags = 0; // W_FILE_FL_READ_VISIBILITY
    outhdr.number_of_waypoints = hdr.number_of_waypoints;

    // map name required or metamod plugin thinks its an invalid file
    // the map name is also compared to the current map name (case senstive)
    string mapname = hdr.mapname.size() ? hdr.mapname : fname;
    strncpy(outhdr.mapname, mapname.c_str(), 32);
    outhdr.mapname[31] = 0;

    FILE* outfile = fopen(outpath.c_str(), "wb");
    if (!outfile) {
        printf("Failed to open output file for writing\n");
        return;
    }

    fwrite(&outhdr, sizeof(WAYPOINT_HDR), 1, outfile);

    for (int i = 0; i < hdr.number_of_waypoints; i++)
    {
        CWaypoint& inputWpt = input_wpts[i];
        WAYPOINT wpt;
        wpt.flags = inputWpt.m_iFlags;
        wpt.origin = inputWpt.m_vOrigin;
        //printf("Write waypoint %d, %d at (%d %d %d)\n", i, wpt.flags, (int)wpt.origin.x, (int)wpt.origin.y, (int)wpt.origin.z);

        fwrite(&wpt, sizeof(WAYPOINT), 1, outfile);
    }

    for (int i = 0; i < hdr.number_of_waypoints; i++)
    {
        CWaypoint& inputWpt = input_wpts[i];

        int16_t numPaths = inputWpt.m_PathsTo.size();
        fwrite(&numPaths, sizeof(int16_t), 1, outfile);

        for (int k = 0; k < numPaths; k++)
        {
            int16_t pathIndex = inputWpt.m_PathsTo[k];
            fwrite(&pathIndex, sizeof(pathIndex), 1, outfile);
        }
    }

    printf("Wrote %s\n", outpath.c_str());

    fclose(outfile);
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        printf("RCBot waypoint converter\n\n");
        printf("This tool converts an .rcwa file to .rcw (angelscript -> metamod waypoint file).\n\n");
        printf("Usage: wptconv input.rcwa\n");
        return 0;
    }

    string path = argv[1];
    if (toLowerCase(path).rfind(".rcwa") != path.length() - 5) {
        printf("File missing .rcwa extension: %s\n", path.c_str());
        return 0;
    }

    if (!fileExists(path)) {
        printf("Failed to open file: %s\n", path.c_str());
        return 0;
    }

    convert_to_rcw(path);

    return 0;
}