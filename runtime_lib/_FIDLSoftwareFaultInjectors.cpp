// DO NOT MODIFY
#include "_SoftwareFaultInjectors.cpp"

/*********************
 * DEFAULT INJECTORS *
 *********************/

// BufferOverflow(API)
static RegisterFaultInjector _API_BufferOverflowFIDLInjector("BufferOverflow(API)", new ChangeValueInjector(45, false));

// BufferOverflowMalloc(Data)
static RegisterFaultInjector _Data_BufferOverflowMallocFIDLInjector("BufferOverflowMalloc(Data)", new ChangeValueInjector(-40, false));

// BufferOverflowMemmove(Data)
static RegisterFaultInjector _Data_BufferOverflowMemmoveFIDLInjector("BufferOverflowMemmove(Data)", new ChangeValueInjector(45, false));

// BufferUnderflow(API)
static RegisterFaultInjector _API_BufferUnderflowFIDLInjector("BufferUnderflow(API)", new ChangeValueInjector(-40, false));

// CPUHog(Res)
static RegisterFaultInjector _Res_CPUHogFIDLInjector("CPUHog(Res)", new SleepInjector());

// DataCorruption(Data)
static RegisterFaultInjector _Data_DataCorruptionFIDLInjector("DataCorruption(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// DeadLock(MPI)
static RegisterFaultInjector _MPI_DeadLockFIDLInjector("DeadLock(MPI)", BitCorruptionInjector::getBitCorruptionInjector());

// DeadLock(Res)
static RegisterFaultInjector _Res_DeadLockFIDLInjector("DeadLock(Res)", new PthreadDeadLockInjector());

// InappropriateClose(API)
static RegisterFaultInjector _API_InappropriateCloseFIDLInjector("InappropriateClose(API)", new InappropriateCloseInjector(true));

// IncorrectOutput(API)
static RegisterFaultInjector _API_IncorrectOutputFIDLInjector("IncorrectOutput(API)", BitCorruptionInjector::getBitCorruptionInjector());

// InvalidMessage(MPI)
static RegisterFaultInjector _MPI_InvalidMessageFIDLInjector("InvalidMessage(MPI)", new ChangeValueInjector(1024, false));

// InvalidPointer(Res)
static RegisterFaultInjector _Res_InvalidPointerFIDLInjector("InvalidPointer(Res)", BitCorruptionInjector::getBitCorruptionInjector());

// InvalidSender(MPI)
static RegisterFaultInjector _MPI_InvalidSenderFIDLInjector("InvalidSender(MPI)", BitCorruptionInjector::getBitCorruptionInjector());

// LowMemory(Res)
static RegisterFaultInjector _Res_LowMemoryFIDLInjector("LowMemory(Res)", new MemoryExhaustionInjector(false));

// MemoryExhaustion(Res)
static RegisterFaultInjector _Res_MemoryExhaustionFIDLInjector("MemoryExhaustion(Res)", new MemoryExhaustionInjector(true));

// MemoryLeak(Res)
static RegisterFaultInjector _Res_MemoryLeakFIDLInjector("MemoryLeak(Res)", new MemoryLeakInjector());

// NoAck(MPI)
static RegisterFaultInjector _MPI_NoAckFIDLInjector("NoAck(MPI)", new HangInjector());

// NoClose(API)
static RegisterFaultInjector _API_NoCloseFIDLInjector("NoClose(API)", new InappropriateCloseInjector(false));

// NoDrain(MPI)
static RegisterFaultInjector _MPI_NoDrainFIDLInjector("NoDrain(MPI)", new ChangeValueInjector(5000, true));

// NoMessage(MPI)
static RegisterFaultInjector _MPI_NoMessageFIDLInjector("NoMessage(MPI)", new HangInjector());

// NoOpen(API)
static RegisterFaultInjector _API_NoOpenFIDLInjector("NoOpen(API)", BitCorruptionInjector::getBitCorruptionInjector());

// NoOutput(API)
static RegisterFaultInjector _API_NoOutputFIDLInjector("NoOutput(API)", new HangInjector());

// PacketStorm(MPI)
static RegisterFaultInjector _MPI_PacketStormFIDLInjector("PacketStorm(MPI)", new ChangeValueInjector(-40, false));

// RaceCondition(Timing)
static RegisterFaultInjector _Timing_RaceConditionFIDLInjector("RaceCondition(Timing)", new PthreadRaceConditionInjector());

// StalePointer(Res)
static RegisterFaultInjector _Res_StalePointerFIDLInjector("StalePointer(Res)", new StalePointerInjector());

// ThreadKiller(Res)
static RegisterFaultInjector _Res_ThreadKillerFIDLInjector("ThreadKiller(Res)", new PthreadThreadKillerInjector());

// UnderAccumulator(Res)
static RegisterFaultInjector _Res_UnderAccumulatorFIDLInjector("UnderAccumulator(Res)", new ChangeValueInjector(45, false));

// WrongAPI(API)
static RegisterFaultInjector _API_WrongAPIFIDLInjector("WrongAPI(API)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongDestination(Data)
static RegisterFaultInjector _Data_WrongDestinationFIDLInjector("WrongDestination(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongMode(API)
static RegisterFaultInjector _API_WrongModeFIDLInjector("WrongMode(API)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongPointer(Data)
static RegisterFaultInjector _Data_WrongPointerFIDLInjector("WrongPointer(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongRetrievedAddress(IO)
static RegisterFaultInjector _IO_WrongRetrievedAddressFIDLInjector("WrongRetrievedAddress(IO)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongRetrievedFormat(IO)
static RegisterFaultInjector _IO_WrongRetrievedFormatFIDLInjector("WrongRetrievedFormat(IO)", new WrongFormatInjector());

// WrongSavedAddress(IO)
static RegisterFaultInjector _IO_WrongSavedAddressFIDLInjector("WrongSavedAddress(IO)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongSavedFormat(IO)
static RegisterFaultInjector _IO_WrongSavedFormatFIDLInjector("WrongSavedFormat(IO)", new WrongFormatInjector());

// WrongSource(Data)
static RegisterFaultInjector _Data_WrongSourceFIDLInjector("WrongSource(Data)", BitCorruptionInjector::getBitCorruptionInjector());

/********************
 * CUSTOM INJECTORS *
 ********************/

