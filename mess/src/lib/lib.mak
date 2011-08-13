###########################################################################
#
#   lib.mak
#
#   MAME dependent library makefile
#
#   Copyright Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


LIBSRC = $(SRC)/lib
LIBOBJ = $(OBJ)/lib

OBJDIRS += \
	$(LIBOBJ)/util \
	$(LIBOBJ)/expat \
	$(LIBOBJ)/formats \
	$(LIBOBJ)/zlib \
	$(LIBOBJ)/p7zip \
	$(LIBOBJ)/miniupnpc-1.4.20100609 \
	$(LIBOBJ)/RakNet \
	$(LIBOBJ)/softfloat \
	$(LIBOBJ)/cothread \



#-------------------------------------------------
# utility library objects
#-------------------------------------------------

UTILOBJS = \
	$(LIBOBJ)/util/astring.o \
	$(LIBOBJ)/util/avcomp.o \
	$(LIBOBJ)/util/aviio.o \
	$(LIBOBJ)/util/bitmap.o \
	$(LIBOBJ)/util/cdrom.o \
	$(LIBOBJ)/util/chd.o \
	$(LIBOBJ)/util/corefile.o \
	$(LIBOBJ)/util/corestr.o \
	$(LIBOBJ)/util/coreutil.o \
	$(LIBOBJ)/util/harddisk.o \
	$(LIBOBJ)/util/huffman.o \
	$(LIBOBJ)/util/jedparse.o \
	$(LIBOBJ)/util/md5.o \
	$(LIBOBJ)/util/opresolv.o \
	$(LIBOBJ)/util/options.o \
	$(LIBOBJ)/util/palette.o \
	$(LIBOBJ)/util/png.o \
	$(LIBOBJ)/util/pool.o \
	$(LIBOBJ)/util/sha1.o \
	$(LIBOBJ)/util/tagmap.o \
	$(LIBOBJ)/util/unicode.o \
	$(LIBOBJ)/util/unzip.o \
	$(LIBOBJ)/util/vbiparse.o \
	$(LIBOBJ)/util/xmlfile.o \
	$(LIBOBJ)/util/zippath.o \

$(OBJ)/libutil.a: $(UTILOBJS)

MINIUPNPCOBJS = \
	$(LIBOBJ)/miniupnpc-1.4.20100609/connecthostport.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/igd_desc_parse.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/minissdpc.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/minisoap.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/miniupnpc.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/miniwget.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/minixml.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/upnpc.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/upnpcommands.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/upnperrors.o \
	$(LIBOBJ)/miniupnpc-1.4.20100609/upnpreplyparse.o \

$(OBJ)/libminiupnpc.a: $(MINIUPNPCOBJS)

$(LIBOBJ)/miniupnpc-1.4.20100609/%.o: $(LIBSRC)/miniupnpc-1.4.20100609/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCOMFLAGS) $(CONLYFLAGS) -c $< -o $@

RAKNETOBJS = \
$(LIBOBJ)/RakNet/BitStream.o \
$(LIBOBJ)/RakNet/CCRakNetSlidingWindow.o \
$(LIBOBJ)/RakNet/CCRakNetUDT.o \
$(LIBOBJ)/RakNet/CheckSum.o \
$(LIBOBJ)/RakNet/CommandParserInterface.o \
$(LIBOBJ)/RakNet/ConnectionGraph2.o \
$(LIBOBJ)/RakNet/ConsoleServer.o \
$(LIBOBJ)/RakNet/DataCompressor.o \
$(LIBOBJ)/RakNet/DirectoryDeltaTransfer.o \
$(LIBOBJ)/RakNet/DS_BytePool.o \
$(LIBOBJ)/RakNet/DS_ByteQueue.o \
$(LIBOBJ)/RakNet/DS_HuffmanEncodingTree.o \
$(LIBOBJ)/RakNet/DS_Table.o \
$(LIBOBJ)/RakNet/EmailSender.o \
$(LIBOBJ)/RakNet/EncodeClassName.o \
$(LIBOBJ)/RakNet/EpochTimeToString.o \
$(LIBOBJ)/RakNet/FileList.o \
$(LIBOBJ)/RakNet/FileListTransfer.o \
$(LIBOBJ)/RakNet/FileOperations.o \
$(LIBOBJ)/RakNet/FormatString.o \
$(LIBOBJ)/RakNet/FullyConnectedMesh2.o \
$(LIBOBJ)/RakNet/Getche.o \
$(LIBOBJ)/RakNet/Gets.o \
$(LIBOBJ)/RakNet/GetTime.o \
$(LIBOBJ)/RakNet/gettimeofday.o \
$(LIBOBJ)/RakNet/GridSectorizer.o \
$(LIBOBJ)/RakNet/HTTPConnection.o \
$(LIBOBJ)/RakNet/IncrementalReadInterface.o \
$(LIBOBJ)/RakNet/Itoa.o \
$(LIBOBJ)/RakNet/LinuxStrings.o \
$(LIBOBJ)/RakNet/LogCommandParser.o \
$(LIBOBJ)/RakNet/MessageFilter.o \
$(LIBOBJ)/RakNet/NatPunchthroughClient.o \
$(LIBOBJ)/RakNet/NatPunchthroughServer.o \
$(LIBOBJ)/RakNet/NatTypeDetectionClient.o \
$(LIBOBJ)/RakNet/NatTypeDetectionCommon.o \
$(LIBOBJ)/RakNet/NatTypeDetectionServer.o \
$(LIBOBJ)/RakNet/NetworkIDManager.o \
$(LIBOBJ)/RakNet/NetworkIDObject.o \
$(LIBOBJ)/RakNet/PacketConsoleLogger.o \
$(LIBOBJ)/RakNet/PacketFileLogger.o \
$(LIBOBJ)/RakNet/PacketizedTCP.o \
$(LIBOBJ)/RakNet/PacketLogger.o \
$(LIBOBJ)/RakNet/PacketOutputWindowLogger.o \
$(LIBOBJ)/RakNet/PluginInterface2.o \
$(LIBOBJ)/RakNet/RakMemoryOverride.o \
$(LIBOBJ)/RakNet/RakNetCommandParser.o \
$(LIBOBJ)/RakNet/RakNetSocket.o \
$(LIBOBJ)/RakNet/RakNetStatistics.o \
$(LIBOBJ)/RakNet/RakNetTransport2.o \
$(LIBOBJ)/RakNet/RakNetTypes.o \
$(LIBOBJ)/RakNet/RakPeer.o \
$(LIBOBJ)/RakNet/RakSleep.o \
$(LIBOBJ)/RakNet/RakString.o \
$(LIBOBJ)/RakNet/RakThread.o \
$(LIBOBJ)/RakNet/Rand.o \
$(LIBOBJ)/RakNet/rdlmalloc.o \
$(LIBOBJ)/RakNet/ReadyEvent.o \
$(LIBOBJ)/RakNet/ReliabilityLayer.o \
$(LIBOBJ)/RakNet/ReplicaManager3.o \
$(LIBOBJ)/RakNet/Router2.o \
$(LIBOBJ)/RakNet/RPC4Plugin.o \
$(LIBOBJ)/RakNet/SecureHandshake.o \
$(LIBOBJ)/RakNet/SendToThread.o \
$(LIBOBJ)/RakNet/SHA1.o \
$(LIBOBJ)/RakNet/SignaledEvent.o \
$(LIBOBJ)/RakNet/SimpleMutex.o \
$(LIBOBJ)/RakNet/SocketLayer.o \
$(LIBOBJ)/RakNet/StringCompressor.o \
$(LIBOBJ)/RakNet/StringTable.o \
$(LIBOBJ)/RakNet/SuperFastHash.o \
$(LIBOBJ)/RakNet/TableSerializer.o \
$(LIBOBJ)/RakNet/TCPInterface.o \
$(LIBOBJ)/RakNet/TeamBalancer.o \
$(LIBOBJ)/RakNet/TelnetTransport.o \
$(LIBOBJ)/RakNet/ThreadsafePacketLogger.o \
$(LIBOBJ)/RakNet/UDPForwarder.o \
$(LIBOBJ)/RakNet/UDPProxyClient.o \
$(LIBOBJ)/RakNet/UDPProxyCoordinator.o \
$(LIBOBJ)/RakNet/UDPProxyServer.o \
$(LIBOBJ)/RakNet/VariableDeltaSerializer.o \
$(LIBOBJ)/RakNet/VariableListDeltaTracker.o \
$(LIBOBJ)/RakNet/VariadicSQLParser.o \
$(LIBOBJ)/RakNet/WSAStartupSingleton.o \
$(LIBOBJ)/RakNet/_FindFirst.o

$(OBJ)/libraknet.a: $(RAKNETOBJS)

$(LIBOBJ)/RakNet/%.o: $(LIBSRC)/RakNet/%.cpp | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCOMFLAGS) -c $< -o $@

#-------------------------------------------------
# expat library objects
#-------------------------------------------------

EXPATOBJS = \
	$(LIBOBJ)/expat/xmlparse.o \
	$(LIBOBJ)/expat/xmlrole.o \
	$(LIBOBJ)/expat/xmltok.o

$(OBJ)/libexpat.a: $(EXPATOBJS)

$(LIBOBJ)/expat/%.o: $(LIBSRC)/expat/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCOMFLAGS) $(CONLYFLAGS) -c $< -o $@



#-------------------------------------------------
# formats library objects
#-------------------------------------------------

FORMATSOBJS = \
	$(LIBOBJ)/formats/cassimg.o 	\
	$(LIBOBJ)/formats/flopimg.o		\
	$(LIBOBJ)/formats/imageutl.o	\
	$(LIBOBJ)/formats/ioprocs.o		\
	$(LIBOBJ)/formats/basicdsk.o	\
	$(LIBOBJ)/formats/a26_cas.o		\
	$(LIBOBJ)/formats/ace_tap.o		\
	$(LIBOBJ)/formats/ami_dsk.o		\
	$(LIBOBJ)/formats/ap2_dsk.o		\
	$(LIBOBJ)/formats/apf_apt.o		\
	$(LIBOBJ)/formats/apridisk.o	\
	$(LIBOBJ)/formats/ap_dsk35.o	\
	$(LIBOBJ)/formats/atari_dsk.o	\
	$(LIBOBJ)/formats/atarist_dsk.o	\
	$(LIBOBJ)/formats/atom_tap.o	\
	$(LIBOBJ)/formats/cbm_tap.o		\
	$(LIBOBJ)/formats/cgen_cas.o	\
	$(LIBOBJ)/formats/coco_cas.o	\
	$(LIBOBJ)/formats/coco_dsk.o	\
	$(LIBOBJ)/formats/comx35_dsk.o	\
	$(LIBOBJ)/formats/coupedsk.o	\
	$(LIBOBJ)/formats/cpis_dsk.o	\
	$(LIBOBJ)/formats/cqm_dsk.o		\
	$(LIBOBJ)/formats/csw_cas.o		\
	$(LIBOBJ)/formats/d64_dsk.o		\
	$(LIBOBJ)/formats/d81_dsk.o		\
	$(LIBOBJ)/formats/d88_dsk.o		\
	$(LIBOBJ)/formats/dim_dsk.o		\
	$(LIBOBJ)/formats/dsk_dsk.o		\
	$(LIBOBJ)/formats/fdi_dsk.o		\
	$(LIBOBJ)/formats/fm7_cas.o		\
	$(LIBOBJ)/formats/fmsx_cas.o	\
	$(LIBOBJ)/formats/g64_dsk.o		\
	$(LIBOBJ)/formats/gtp_cas.o		\
	$(LIBOBJ)/formats/hect_dsk.o	\
	$(LIBOBJ)/formats/hect_tap.o	\
	$(LIBOBJ)/formats/imd_dsk.o		\
	$(LIBOBJ)/formats/kim1_cas.o	\
	$(LIBOBJ)/formats/lviv_lvt.o	\
	$(LIBOBJ)/formats/msx_dsk.o		\
	$(LIBOBJ)/formats/mz_cas.o		\
	$(LIBOBJ)/formats/nes_dsk.o		\
	$(LIBOBJ)/formats/orao_cas.o	\
	$(LIBOBJ)/formats/oric_dsk.o	\
	$(LIBOBJ)/formats/oric_tap.o	\
	$(LIBOBJ)/formats/p6001_cas.o	\
	$(LIBOBJ)/formats/pc_dsk.o		\
	$(LIBOBJ)/formats/pmd_pmd.o		\
	$(LIBOBJ)/formats/primoptp.o	\
	$(LIBOBJ)/formats/rk_cas.o		\
	$(LIBOBJ)/formats/smx_dsk.o		\
	$(LIBOBJ)/formats/sorc_dsk.o	\
	$(LIBOBJ)/formats/sord_cas.o	\
	$(LIBOBJ)/formats/svi_cas.o		\
	$(LIBOBJ)/formats/svi_dsk.o		\
	$(LIBOBJ)/formats/td0_dsk.o		\
	$(LIBOBJ)/formats/thom_cas.o	\
	$(LIBOBJ)/formats/thom_dsk.o	\
	$(LIBOBJ)/formats/ti99_dsk.o	\
	$(LIBOBJ)/formats/trd_dsk.o		\
	$(LIBOBJ)/formats/trs_cas.o		\
	$(LIBOBJ)/formats/trs_dsk.o		\
	$(LIBOBJ)/formats/tzx_cas.o		\
	$(LIBOBJ)/formats/uef_cas.o		\
	$(LIBOBJ)/formats/vg5k_cas.o	\
	$(LIBOBJ)/formats/vt_cas.o		\
	$(LIBOBJ)/formats/vt_dsk.o		\
	$(LIBOBJ)/formats/vtech1_dsk.o	\
	$(LIBOBJ)/formats/wavfile.o		\
	$(LIBOBJ)/formats/x1_tap.o		\
	$(LIBOBJ)/formats/z80ne_dsk.o	\
	$(LIBOBJ)/formats/zx81_p.o		\

$(OBJ)/libformats.a: $(FORMATSOBJS)



#-------------------------------------------------
# zlib library objects
#-------------------------------------------------

ZLIBOBJS = \
	$(LIBOBJ)/zlib/adler32.o \
	$(LIBOBJ)/zlib/compress.o \
	$(LIBOBJ)/zlib/crc32.o \
	$(LIBOBJ)/zlib/deflate.o \
	$(LIBOBJ)/zlib/gzio.o \
	$(LIBOBJ)/zlib/inffast.o \
	$(LIBOBJ)/zlib/inflate.o \
	$(LIBOBJ)/zlib/infback.o \
	$(LIBOBJ)/zlib/inftrees.o \
	$(LIBOBJ)/zlib/trees.o \
	$(LIBOBJ)/zlib/uncompr.o \
	$(LIBOBJ)/zlib/zutil.o

$(OBJ)/libz.a: $(ZLIBOBJS)

$(LIBOBJ)/zlib/%.o: $(LIBSRC)/zlib/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCOMFLAGS) $(CONLYFLAGS) -c $< -o $@

#-------------------------------------------------
# p7zip library objects
#-------------------------------------------------

P7ZIPOBJS = \
	$(LIBOBJ)/p7zip/LzFind.o \
	$(LIBOBJ)/p7zip/LzmaDec.o \
	$(LIBOBJ)/p7zip/LzmaEnc.o \
	$(LIBOBJ)/p7zip/Lzma2Dec.o \
	$(LIBOBJ)/p7zip/Lzma2Enc.o

$(OBJ)/libp7zip.a: $(P7ZIPOBJS)

$(LIBOBJ)/p7zip/%.o: $(LIBSRC)/p7zip/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCOMFLAGS) $(CONLYFLAGS) -c $< -o $@

#-------------------------------------------------
# SoftFloat library objects
#-------------------------------------------------

PROCESSOR_H = $(LIBSRC)/softfloat/processors/mamesf.h
SOFTFLOAT_MACROS = $(LIBSRC)/softfloat/softfloat/bits64/softfloat-macros

SOFTFLOATOBJS = \
	$(LIBOBJ)/softfloat/softfloat.o

$(OBJ)/libsoftfloat.a: $(SOFTFLOATOBJS)

$(LIBOBJ)/softfloat/softfloat.o: $(LIBSRC)/softfloat/softfloat.c $(LIBSRC)/softfloat/softfloat.h $(LIBSRC)/softfloat/softfloat-macros $(LIBSRC)/softfloat/softfloat-specialize



#-------------------------------------------------
# cothread library objects
#-------------------------------------------------

COTHREADOBJS = \
	$(LIBOBJ)/cothread/libco.o

$(OBJ)/libco.a: $(COTHREADOBJS)

$(LIBOBJ)/cothread/%.o: $(LIBSRC)/cothread/%.c | $(OSPREBUILD)
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CCOMFLAGS) -c -fomit-frame-pointer $< -o $@


