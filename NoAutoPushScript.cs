using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using GTA;

namespace NikoNoAutoPush {
    public class NoAutoPushScript : Script {
        private readonly byte[] getterBytes = new byte[6];
        private readonly StringBuilder helperStatus = new StringBuilder(256);
        private bool helperUnavailable;
        private bool statusChecked;
        private int lastStatusTime;
        private string lastReportedError;

        [DllImport("kernel32.dll")]
        private static extern IntPtr GetCurrentProcess();

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool ReadProcessMemory(IntPtr process, IntPtr address, out int value, int size, out IntPtr bytesRead);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool ReadProcessMemory(IntPtr process, IntPtr address, [Out] byte[] buffer, int size, out IntPtr bytesRead);

        [DllImport("NoAutoPushHelper.asi", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int RequestShoveAbort(uint ped, uint task);

        [DllImport("NoAutoPushHelper.asi", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, CharSet = CharSet.Ansi)]
        private static extern int GetShoveAbortStatus(StringBuilder buffer, int capacity);

        public NoAutoPushScript() {
            Interval = 16;
            Tick += OnTick;
        }

        private void OnTick(object sender, EventArgs e) {
            if (helperUnavailable) return;
            if (IntPtr.Size != 4) {
                helperUnavailable = true;
                ReportError("Cancellation disabled: requires a 32-bit process.");
                return;
            }

            try {
                int now = Game.GameTime;
                if (!statusChecked || now < lastStatusTime || now - lastStatusTime >= 1000) {
                    statusChecked = true;
                    lastStatusTime = now;
                    if (GetShoveAbortStatus(helperStatus, helperStatus.Capacity) == 0) {
                        ReportError("Helper status read failed.");
                    } else {
                        string status = helperStatus.ToString();
                        if (status.StartsWith("disabled", StringComparison.Ordinal)) {
                            helperUnavailable = true;
                            ReportError(status);
                            return;
                        }
                        if (status.StartsWith("rejected", StringComparison.Ordinal) ||
                            status.IndexOf("failed", StringComparison.Ordinal) >= 0 ||
                            status.IndexOf("skipped", StringComparison.Ordinal) >= 0) {
                            ReportError(status);
                        }
                    }
                }

                Ped player = Player.Character;
                if (player == null || !Exists(player)) return;
                uint ped = unchecked((uint)player.MemoryAddress);
                if (ped == 0 || !ReadPointer(ped + 0x224, out uint intelligence) || intelligence == 0 ||
                    !ReadPointer(intelligence + 0x40, out uint owner) || owner != ped ||
                    !ReadPointer(intelligence + 0x68, out uint task) || task == 0 ||
                    !ReadPointer(task, out uint table) || table == 0 ||
                    !ReadPointer(table + 12, out uint getter) || getter == 0) return;

                if (!ReadProcessMemory(GetCurrentProcess(), new IntPtr((long)getter), getterBytes,
                        getterBytes.Length, out IntPtr count) || count.ToInt64() != getterBytes.Length) return;
                if (getterBytes[0] == 0xB8 && getterBytes[1] == 0x3E &&
                    getterBytes[2] == 0x01 && getterBytes[3] == 0 &&
                    getterBytes[4] == 0 && getterBytes[5] == 0xC3) {
                    RequestShoveAbort(ped, task);
                }
            } catch (DllNotFoundException error) {
                DisableHelper(error);
            } catch (EntryPointNotFoundException error) {
                DisableHelper(error);
            } catch (BadImageFormatException error) {
                DisableHelper(error);
            }
        }

        private static bool ReadPointer(uint address, out uint value) {
            bool success = ReadProcessMemory(GetCurrentProcess(), new IntPtr((long)address),
                out int pointer, sizeof(int), out IntPtr count) && count.ToInt64() == sizeof(int);
            value = unchecked((uint)pointer);
            return success;
        }

        private void DisableHelper(Exception error) {
            helperUnavailable = true;
            ReportError("Helper unavailable: " + error);
        }

        private void ReportError(string message) {
            if (message == lastReportedError) return;
            string directory = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            File.AppendAllText(Path.Combine(directory, "NikoNoAutoPush.log"),
                DateTime.Now.ToString("s") + " " + message + Environment.NewLine);
            lastReportedError = message;
        }
    }
}
