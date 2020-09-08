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
    [0x1A] = "Read Data By Local ID",
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

local nrc_description = {
    [0x10] = "General reject",
    [0x11] = "Service not supported",
    [0x12] = "Subfunction not supported",
    [0x13] = "Message length or format incorrect",
    [0x21] = "Busy - Repeat request",
    [0x22] = "Conditions not correct",
    [0x23] = "Routine not complete",
    [0x24] = "Request sentence error",
    [0x31] = "Request out of range",
    [0x33] = "Security access denied",
    [0x35] = "Invalid key",
    [0x36] = "Exceed attempts",
    [0x75] = "Illegal Byte Count in block transfer",
    [0x78] = "Busy - Response pending",
    [0x7E] = "Service or Subfunction not supported",
    [0x7F] = "Service or Subfunction not supported",
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

--KWP 1A
local rdbci_dict_description = {
    ["90"] = "VIN",
    ["9b"] = "ECU Identification",
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
    ["c5"] = "Check Flash",
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
