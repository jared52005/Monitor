-- Base file taken from: https://github.com/Stubatiger/can-wireshark-dissector
-- Description of protocol partially taken from: https://github.com/NefMoto/NefMotoOpenSource 
-- constants
local sid_dict_description = {
    [0x01] = "SAEJ1979_ShowCurrentData",
    [0x02] = "SAEJ1979_ShowFreezeFrameData",
    [0x03] = "SAEJ1979_ShowDiagnosticTroubleCodes",
    [0x04] = "SAEJ1979_ClearTroubleCodesAndStoredValues",
    [0x05] = "SAEJ1979_TestResultOxygenSenors",
    [0x06] = "SAEJ1979_TestResultsNonContinuouslyMonitored",
    [0x07] = "SAEJ1979_ShowPendingTroubleCodes",
    [0x08] = "SAEJ1979_SpecialControlMode",
    [0x09] = "SAEJ1979_RequestVehicleInformation",
    [0x0A] = "SAEJ1979_RequestPermanentTroubleCodes",
    [0x10] = "Start Diagnostic Session",
    [0x11] = "ECU Reset",
    [0x12] = "Read Freeze Frame Data",
    [0x13] = "Read Diagnostic Trouble Code",
    [0x14] = "Clear Diagnostic Information",
    [0x17] = "Read Status of DTCs",
    [0x18] = "Read DTCs by Status",
    [0x1A] = "Read ECU Identification",
    [0x20] = "Stop Diagnostic Session",
    [0x21] = "Read Data by Local Identifier",
    [0x22] = "Read Data by Common Identifier",
    [0x23] = "Read Memory by Address",
    [0x25] = "Stop Repeated Data Transmission",
    [0x26] = "Set Data Rates",
    [0x27] = "Security Access",
    [0x28] = "Disable Normal Message Transm.",
    [0x29] = "Enable Normal Message Transm.",
    [0x2c] = "Dynamically define Data Identifier",
    [0x2E] = "Write Data by Common Identifier",
    [0x2F] = "Input Output Control by Common ID",
    [0x30] = "Input Output Control by Local ID",
    [0x31] = "Start Routine by Local Identifier",
    [0x32] = "Stop Routine by Local Identifier",
    [0x33] = "Request Routine Results by Local ID",
    [0x34] = "Request Download",
    [0x35] = "Request Upload",
    [0x36] = "Transfer Data",
    [0x37] = "Request Transfer Exit",
    [0x38] = "Start Routine by Address",
    [0x39] = "Stop Routine by Address",
    [0x3A] = "Request Routine Results by Address",
    [0x3B] = "Write Data by Local Identifier",
    [0x3d] = "Write Memory by Address",
    [0x3E] = "Tester Present",
    [0x81] = "Start Comm",
    [0x82] = "Stop Comm",
    [0x83] = "Access Timing Parameters",
    [0x84] = "Network Configuration",
    [0x85] = "Control DTC Setting",

    [0x7F] = "Negative Response",
}

local nrc_description = {
    [0x10] = "General reject",
    [0x11] = "Service not supported - Invalid format",
    [0x12] = "Subfunction not supported",
    [0x13] = "Message length or format incorrect",
    [0x21] = "Busy - Repeat request",
    [0x22] = "Conditions not correct or Request Sequence Error",
    [0x23] = "Routine not complete or Service in progress",
    [0x24] = "Request sentence error",
    [0x31] = "Request out of range",
    [0x33] = "Security access denied",
    [0x35] = "Invalid key",
    [0x36] = "Exceed Number of Attempts",
    [0x37] = "Required Time Delay not Expired",
    [0x40] = "Download not Accpted",
    [0x41] = "Improper Download Type",
    [0x42] = "Can Not Download To Specified Address",
    [0x43] = "CanNotDownloadNumberOfBytesRequested",
    [0x50] = "Upload Not Accepted",
    [0x51] = "Improper Upload Type",
    [0x52] = "Can Not Upload From Specified Address",
    [0x53] = "Can Not Upload Number of Bytes Requested",
    [0x71] = "Transfer Suspended",
    [0x72] = "Transfer Aborted",
    [0x74] = "Illegal Address in Block Transfer",
    [0x75] = "Illegal Byte Count in block transfer",
    [0x76] = "Illegal Block Transfer Type",
    [0x77] = "Block Transfer Data Checksum Error",
    [0x78] = "Busy - Response pending",
    [0x79] = "Incorrect Byte Count During Block Transfer",
    [0x7E] = "Service or Subfunction not supported",
    [0x7F] = "Service or Subfunction not supported",
    [0x80] = "Service Not Supported In Active Diagnostic Session",
    [0x90] = "No Program",
}

function NegativeResponse_InfoColumn(tvbuf, pktinfo)
    -- `[7F2233] - RDBLI; Security Requested` = Service ID (22 = RDBLI) Negative Response code (33 = Security)
    local preview = tostring(tvbuf:range(0,3))
    local nrc = nrc_description[tvbuf:range(2,1):uint()]
    local sid = tvbuf:range(1,1):uint()
    if nrc == nil then
        nrc= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[sid] .. "; " .. nrc
end

--KWP 10
local sds_dict_description = {
    ["81"] = "Standard Session",
    ["83"] = "End Of Line - Fiat",
    ["84"] = "End Of Line - System Supplier",
    ["85"] = "Programming",
    ["86"] = "Extended",
    ["87"] = "Adjustment Session",
    ["89"] = "Default",
}
function SDS_Baudrate(baudRateByte)
    -- 0x14 = 10400
    local exp = bit32.rrotate(baudRateByte, 0x5);
    exp = bit32.band(exp, 0x7)
    local pow = bit32.lrotate(0x1, exp);
    
    local base = bit32.band(baudRateByte, 0x1F);
    
    local baudRate = 200 * pow * (base + 32)
    return baudRate;
end

function SDS_InfoColumn(tvbuf, pktinfo)
    -- `[1001] - Start Diagnostic Session; Default`
    local pktlen = tvbuf:len()
    local preview = tostring(tvbuf:range(0,2))
    local type = sds_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    if(pktlen == 3) then
        -- Calculate also baudrate
        local baudrateByte = tvbuf:range(2,1):uint()
        local baudrate = SDS_Baudrate(baudrateByte)
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x10] .. "; " .. type .. "Request baudrate: " .. string.upper(baudrate)
    else
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x10] .. "; " .. type
    end
end

--KWP 1A
local rdbci_dict_description = {
    ["81"] = "ECU Identification Scaling Table",
    ["82"] = "ECU Identification Data Table",
    ["86"] = "Vehicle Manufacturer Specific", -- Extended ECU identification number, serial number? SCA insertion requires this field
	["87"] = "Vehicle Manufacturer Spare Part Number",
	["88"] = "Vehicle Manufacturer ECU Software Number",
	["89"] = "Vehicle Manufacturer ECU Software Version Number",
    ["8a"] = "System Supplier",
	["8b"] = "ECU Manufacturing Date",
	["8c"] = "ECU Serial Number",
    --systemSupplierSpecific1 = 0x8D,
    --systemSupplierSpecific2 = 0x8E,
    --systemSupplierSpecific3 = 0x8F,
    ["90"] = "VIN",
    ["9b"] = "ECU Identification",
	["91"] = "Vehicle Manufacturer ECU Hardware Number",
	["92"] = "System Supplier ECU Hardware Number",
	["93"] = "System Supplier ECU Hardware Version Number",
	["94"] = "System Supplier ECU Software Number",
	["95"] = "System Supplier ECU Software Version Number",
	["96"] = "Exhaust Regulation or Type Approval Number",
	["97"] = "System Name or Engine Type",
	["98"] = "Repair Shop Code or Tester Serial Number",
	["99"] = "Programming Date",
	["9a"] = "Calibration Repair Shop Code or Calibration Equipment Serial Number",
	["9b"] = "Calibration Date", -- ECU identification number.  SCA insertion requires this field
	["9c"] = "Calibration Equiment Software Number", -- flash status -  byte:flash status, byte:num flash attempts, byte:num successful flash attempts, byte:status of flash preconditions
	["9d"] = "ECU Installation Date",
    -- vehicleManufacturerSpecific1 = 0x9E,
    -- vehicleManufacturerSpecific2 = 0x9F,
    -- vehicleManufacturerSpecific3 = 0xA0,
    -- vehicleManufacturerSpecific4 = 0xA1,
    -- vehicleManufacturerSpecific5 = 0xA2,
    -- vehicleManufacturerSpecific6 = 0xA3,
    -- vehicleManufacturerSpecific7 = 0xA4,
    -- vehicleManufacturerSpecific8 = 0xA5,
    -- vehicleManufacturerSpecific9 = 0xA6,
    -- vehicleManufacturerSpecific10 = 0xA7,
    -- vehicleManufacturerSpecific11 = 0xA8,
    -- vehicleManufacturerSpecific12 = 0xA9,
    -- vehicleManufacturerSpecific13 = 0xAA,
    -- vehicleManufacturerSpecific14 = 0xAB,
    -- vehicleManufacturerSpecific15 = 0xAC,
    -- vehicleManufacturerSpecific16 = 0xAD,
    -- vehicleManufacturerSpecific17 = 0xAE,
    -- vehicleManufacturerSpecific18 = 0xAF,
    -- systemSupplierSpecific4 = 0xB0,
    -- systemSupplierSpecific5 = 0xB1,
    -- systemSupplierSpecific6 = 0xB2,
    -- systemSupplierSpecific7 = 0xB3,
    -- systemSupplierSpecific8 = 0xB4,
    -- systemSupplierSpecific9 = 0xB5,
    -- systemSupplierSpecific10 = 0xB6,
    -- systemSupplierSpecific11 = 0xB7,
    -- systemSupplierSpecific12 = 0xB8,
    -- systemSupplierSpecific13 = 0xB9,
    -- systemSupplierSpecific14 = 0xBA,
    -- systemSupplierSpecific15 = 0xBB,
    -- systemSupplierSpecific16 = 0xBC,
    -- systemSupplierSpecific17 = 0xBD,
    -- systemSupplierSpecific18 = 0xBE,
    -- systemSupplierSpecific19 = 0xBF
}

function RDBCI_InfoColumn(tvbuf, pktinfo)
    -- `[1A9B] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
    local preview = tostring(tvbuf:range(0,2))
    local type = rdbci_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x1A] .. "; " .. type
end

--KWP 20
function EDS_InfoColumn(tvbuf, pktinfo)
    -- `[20] - End Diagnostic Session`
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x20]
end

--KWP 27
function SA_InfoColumn(tvbuf, pktinfo)
    -- `[2701] - Seceurity access`
    local preview = tostring(tvbuf:range(0,2))
    local type = "Level: " .. tostring(tvbuf:range(1,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x27] .. "; " .. type
end

--KWP 31
local rc_dict_description = {
    ["c4"] = "Erase Flash",
    ["c5"] = "Validate Flash Checksum",
}
function RC_InfoColumn(tvbuf, pktinfo)
    -- `[31C4] - Tester Present; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = rc_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x31] .. "; " .. type
end

function RRRBLID_InfoColumn(tvbuf, pktinfo)
    -- `[33C4] - RRRBLID; `
    local preview = tostring(tvbuf:range(0,2))
    local type = rc_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x33] .. "; " .. type
end

--KWP 34
function RD_InfoColumn(tvbuf, pktinfo)
    sid = tvbuf:range(0,1):uint()
    if(sid == 0x34 ) then
        -- `[34 1e c0 00 01 01 3e 00] - Request download <address> [<length>]`
        local preview = tostring(tvbuf:range(0,1))
        local address = tostring(tvbuf:range(1,3))
        local length = tostring(tvbuf:range(5,3))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x34] .. "; " .. string.upper(address) .. " [" .. string.upper(length) .. "]"
    else
        local preview = tostring(tvbuf:range(0,1))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x34]
    end
end

--KWP 35
function RU_InfoColumn(tvbuf, pktinfo)
    sid = tvbuf:range(0,1):uint()
    if(sid == 0x35 ) then
        -- `[35 1e c0 00 01 01 3e 00] - Request upload <address> [<length>]`
        local preview = tostring(tvbuf:range(0,1))
        local address = tostring(tvbuf:range(1,3))
        local length = tostring(tvbuf:range(5,3))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x35] .. "; " .. string.upper(address) .. " [" .. string.upper(length) .. "]"
    else
        local preview = tostring(tvbuf:range(0,1))
        pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x35]
    end
end

--KWP 36
function T_InfoColumn(tvbuf, pktinfo)
    -- `[36] - Request transfer exit
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "...] - " ..  sid_dict_description[0x36]
end

--KWP 37
function RTE_InfoColumn(tvbuf, pktinfo)
    -- `[37] - Request transfer exit
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x37]
end

--KWP 3E
function TP_InfoColumn(tvbuf, pktinfo)
    -- `[3E] - Tester Present; Default`
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x3E]
end

--KWP 81
function SC_InfoColumn(tvbuf, pktinfo)
    -- `[81] - Start communication
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x81]
end

--KWP 82
function EC_InfoColumn(tvbuf, pktinfo)
    -- `[82] - End communication
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x82]
end

local sid_dict_methods = {
    [0x10] = SDS_InfoColumn,
    [0x1A] = RDBCI_InfoColumn,
    [0x20] = EDS_InfoColumn,
    [0x27] = SA_InfoColumn,
    [0x31] = RC_InfoColumn,
    [0x33] = RRRBLID_InfoColumn,
    [0x34] = RD_InfoColumn,
    [0x35] = RU_InfoColumn,
    [0x36] = T_InfoColumn,
    [0x37] = RTE_InfoColumn,
    [0x3E] = TP_InfoColumn,
    [0x81] = SC_InfoColumn,
    [0x82] = EC_InfoColumn,
}

local positive_response_mask = 0x40
local negative_response = 0x7F

-- fields
local length = ProtoField.new("Payload Length", "kwp2k.length", ftypes.UINT8,nil, base.DEC)
local sid = ProtoField.uint8("kwp2k.sid", "Service ID", base.HEX, sid_dict_description)
local sid_ispos = ProtoField.uint8("kwp2k.is_positive", "Pos. Res; Service ID", base.HEX, sid_dict_description)
local sid_isneg = ProtoField.uint8("kwp2k.is_negative", "Negative Response", base.HEX, sid_dict_description)
local data = ProtoField.new("Data", "kwp2k.data", ftypes.BYTES)
local nrc = ProtoField.uint8("kwp2k.nrc", "Negative Response Code", base.HEX, nrc_description)

-- declare dissector
local kwp2000_dissector = Proto.new("kwp2k", "KWP2000")

kwp2000_dissector.fields = {
	length,
    sid,
    sid_ispos,
    sid_isneg,
    data,
    nrc,
}

pktState = {}

function kwp2000_dissector.dissector(tvbuf,pktinfo,root)
    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("KWP2000")
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(kwp2000_dissector, tvbuf:range(0,pktlen))

    tree:add(length, tvbuf:len())

    local cur_sid = tvbuf:range(0,1):uint()
    pktlen = tvbuf:len()

    --Is negative response?
    if cur_sid == negative_response then
        tree:add(sid_isneg, tvbuf:range(0,1))
        tree:add(sid, tvbuf:range(1,1))
        tree:add(nrc, tvbuf:range(2,1))
        NegativeResponse_InfoColumn(tvbuf, pktinfo)
    --Is positive response (SID | 0x40)?
    elseif bit32.btest(cur_sid, positive_response_mask) then
        tree:add(sid_ispos, tvbuf:range(0,1))
        tree:add(data, tvbuf:range(0,pktlen))
        if sid_dict_methods[cur_sid - 0x40] ~= nul then
            sid_dict_methods[cur_sid - 0x40](tvbuf, pktinfo)
        end
    --Is request
    elseif sid_dict_description[cur_sid] ~= nil then
        tree:add(sid, tvbuf:range(0,1))
        tree:add(data, tvbuf:range(0,pktlen))
		if sid_dict_methods[cur_sid] ~= nul then
            sid_dict_methods[cur_sid](tvbuf, pktinfo)
        end
    end
end

--Asign to protocol 0x93: VWTP2.0
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0x93, kwp2000_dissector)
