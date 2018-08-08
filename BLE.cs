using UnityEngine;
using System.Collections;
using System;
using System.Runtime.InteropServices;

public class BLE : MonoBehaviour {
    [DllImport ("Unity3D_BLE")]
    private static extern void BLEInitialise();

    [DllImport ("Unity3D_BLE")]
    private static extern void BLEDeInitialise();

    void Log(string message) {
	Debug.Log("BLE: " + message);
    }

    void Start () {
	Log("Initialise");
	BLEInitialise();
	Log("Initialise...done.");
    }

    void OnDisable() {
	Log("DeInitialise");
	BLEDeInitialise();
    }
}
