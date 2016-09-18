// DO NOT MODIFY
#include "_SoftwareFaultInjectors.cpp"

/*********************
 * DEFAULT INJECTORS *
 *********************/

// RaceCondition(Timing)
static RegisterFaultInjector _Timing_RaceConditionFIDLInjector("RaceCondition(Timing)", new PthreadRaceConditionInjector());

// MemoryLeak(Res)
static RegisterFaultInjector _Res_MemoryLeakFIDLInjector("MemoryLeak(Res)", new MemoryLeakInjector());

// CPUHog(Res)
static RegisterFaultInjector _Res_CPUHogFIDLInjector("CPUHog(Res)", new SleepInjector());

// LowMemory(Res)
static RegisterFaultInjector _Res_LowMemoryFIDLInjector("LowMemory(Res)", new MemoryExhaustionInjector(false));

// BufferUnderflow(API)
static RegisterFaultInjector _API_BufferUnderflowFIDLInjector("BufferUnderflow(API)", new ChangeValueInjector(-40, false));

// DataCorruption(Data)
static RegisterFaultInjector _Data_DataCorruptionFIDLInjector("DataCorruption(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// StalePointer(Res)
static RegisterFaultInjector _Res_StalePointerFIDLInjector("StalePointer(Res)", new StalePointerInjector());

// NoOpen(API)
static RegisterFaultInjector _API_NoOpenFIDLInjector("NoOpen(API)", BitCorruptionInjector::getBitCorruptionInjector());

// MemoryExhaustion(Res)
static RegisterFaultInjector _Res_MemoryExhaustionFIDLInjector("MemoryExhaustion(Res)", new MemoryExhaustionInjector(true));

// WrongSource(Data)
static RegisterFaultInjector _Data_WrongSourceFIDLInjector("WrongSource(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// DeadLock(MPI)
static RegisterFaultInjector _MPI_DeadLockFIDLInjector("DeadLock(MPI)", BitCorruptionInjector::getBitCorruptionInjector());

// InvalidMessage(MPI)
static RegisterFaultInjector _MPI_InvalidMessageFIDLInjector("InvalidMessage(MPI)", new ChangeValueInjector(1024, false));

// InappropriateClose(API)
static RegisterFaultInjector _API_InappropriateCloseFIDLInjector("InappropriateClose(API)", new InappropriateCloseInjector(true));

// WrongSavedFormat(IO)
static RegisterFaultInjector _IO_WrongSavedFormatFIDLInjector("WrongSavedFormat(IO)", new WrongFormatInjector());

// WrongAPI(API)
static RegisterFaultInjector _API_WrongAPIFIDLInjector("WrongAPI(API)", BitCorruptionInjector::getBitCorruptionInjector());

// InvalidSender(MPI)
static RegisterFaultInjector _MPI_InvalidSenderFIDLInjector("InvalidSender(MPI)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongSavedAddress(IO)
static RegisterFaultInjector _IO_WrongSavedAddressFIDLInjector("WrongSavedAddress(IO)", BitCorruptionInjector::getBitCorruptionInjector());

// BufferOverflowMemmove(Data)
static RegisterFaultInjector _Data_BufferOverflowMemmoveFIDLInjector("BufferOverflowMemmove(Data)", new ChangeValueInjector(45, false));

// InvalidPointer(Res)
static RegisterFaultInjector _Res_InvalidPointerFIDLInjector("InvalidPointer(Res)", BitCorruptionInjector::getBitCorruptionInjector());

// WrongPointer(Data)
static RegisterFaultInjector _Data_WrongPointerFIDLInjector("WrongPointer(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// BufferOverflowMalloc(Data)
static RegisterFaultInjector _Data_BufferOverflowMallocFIDLInjector("BufferOverflowMalloc(Data)", new ChangeValueInjector(-40, false));

// UnderAccumulator(Res)
static RegisterFaultInjector _Res_UnderAccumulatorFIDLInjector("UnderAccumulator(Res)", new ChangeValueInjector(45, false));

// DeadLock(Res)
static RegisterFaultInjector _Res_DeadLockFIDLInjector("DeadLock(Res)", new PthreadDeadLockInjector());

// ThreadKiller(Res)
static RegisterFaultInjector _Res_ThreadKillerFIDLInjector("ThreadKiller(Res)", new PthreadThreadKillerInjector());

// NoClose(API)
static RegisterFaultInjector _API_NoCloseFIDLInjector("NoClose(API)", new InappropriateCloseInjector(false));

// WrongDestination(Data)
static RegisterFaultInjector _Data_WrongDestinationFIDLInjector("WrongDestination(Data)", BitCorruptionInjector::getBitCorruptionInjector());

// BufferOverflow(API)
static RegisterFaultInjector _API_BufferOverflowFIDLInjector("BufferOverflow(API)", new ChangeValueInjector(45, false));

// NoAck(MPI)
static RegisterFaultInjector _MPI_NoAckFIDLInjector("NoAck(MPI)", new HangInjector());

// PacketStorm(MPI)
static RegisterFaultInjector _MPI_PacketStormFIDLInjector("PacketStorm(MPI)", new ChangeValueInjector(-40, false));

// WrongMode(API)
static RegisterFaultInjector _API_WrongModeFIDLInjector("WrongMode(API)", BitCorruptionInjector::getBitCorruptionInjector());

// NoDrain(MPI)
static RegisterFaultInjector _MPI_NoDrainFIDLInjector("NoDrain(MPI)", new ChangeValueInjector(5000, true));

// WrongRetrievedFormat(IO)
static RegisterFaultInjector _IO_WrongRetrievedFormatFIDLInjector("WrongRetrievedFormat(IO)", new WrongFormatInjector());

// WrongRetrievedAddress(IO)
static RegisterFaultInjector _IO_WrongRetrievedAddressFIDLInjector("WrongRetrievedAddress(IO)", BitCorruptionInjector::getBitCorruptionInjector());

// IncorrectOutput(API)
static RegisterFaultInjector _API_IncorrectOutputFIDLInjector("IncorrectOutput(API)", BitCorruptionInjector::getBitCorruptionInjector());

// NoMessage(MPI)
static RegisterFaultInjector _MPI_NoMessageFIDLInjector("NoMessage(MPI)", new HangInjector());

// NoOutput(API)
static RegisterFaultInjector _API_NoOutputFIDLInjector("NoOutput(API)", new HangInjector());

/********************
 * CUSTOM INJECTORS *
 ********************/

