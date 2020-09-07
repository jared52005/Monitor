-- Base file taken from: https://github.com/Stubatiger/can-wireshark-dissector
-- constants
local sid_dict_description = {
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

local response_codes = {
    [0x10] = "General reject",
    [0x11] = "Service or Subfunction not supported",
    [0x12] = "Service or Subfunction not supported",
    [0x13] = "Message length or format incorrect",
    [0x21] = "Busy - Repeat request",
    [0x22] = "Conditions not correct",
    [0x24] = "Request sentence error",
    [0x31] = "Out of range",
    [0x33] = "Security access denied",
    [0x35] = "Invalid key",
    [0x36] = "Exceed attempts",
    [0x78] = "Busy - Response pending",
    [0x7E] = "Service or Subfunction not supported",
    [0x7F] = "Service or Subfunction not supported",
}

-- UDS 10
local sds_dict_description = {
    ["89"] = "Default",
    ["85"] = "Programming",
    ["86"] = "Extended",
}
function SDS_InfoColumn(tvbuf, pktinfo)
    -- `[1001] - Start Diagnostic Session; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = sds_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x10] .. "; " .. type
end

--UDS 1A
local rdbci_dict_description = {
    ["f190"] = "VIN",
}

function RDBCI_InfoColumn(tvbuf, pktinfo)
    -- `[1A9B] - RDBLI; VIN` = First 3 bytes of request, Service ID (22 = RDBLI) Subfunciton ID (F190 = VIN)
    local preview = tostring(tvbuf:range(0,2))
    local type = rdbci_dict_description[tostring(tvbuf:range(1,1))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x22] .. "; " .. type
end

--UDS 31
local rc_dict_description = {
    ["0203"] = "Programming Preconditions",
}
function RC_InfoColumn(tvbuf, pktinfo)
    -- `[31010203] - Tester Present; Default`
    local preview = tostring(tvbuf:range(0,2))
    local type = rc_dict_description[tostring(tvbuf:range(2,2))]
    if type == nil then
        type= "???"
    end
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x31] .. "; " .. type
end

--UDS 3E
local tp_dict_description = {
    ["00"] = "Default",
    ["80"] = "Supres response",
}

function TP_InfoColumn(tvbuf, pktinfo)
    -- `[3E00] - Tester Present; Default`
    local preview = tostring(tvbuf:range(0,1))
    pktinfo.cols.info = "[" .. string.upper(preview) .. "] - " ..  sid_dict_description[0x3E]
end

local sid_dict_methods = {
    [0x10] = SDS_InfoColumn,
    [0x1A] = RDBCI_InfoColumn,
    [0x31] = RC_InfoColumn,
    [0x3E] = TP_InfoColumn,
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
        pktinfo.cols.info = "NOK"
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
