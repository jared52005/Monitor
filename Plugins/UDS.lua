-- constants
local sid_dict_description = {
    [0x10] = "Start Diagnostic Session",
    [0x3E] = "Tester Present",
    [0x27] = "Security Access",
    [0x11] = "ECU Reset",
    [0x28] = "Disable Normal Message Transm.",
    [0x29] = "Enable Normal Message Transm.",
    [0x85] = "Control DTC Setting",
    [0x83] = "Access Timing Parameters",
    [0x84] = "Network Configuration",

    [0x22] = "RDBLI",
    [0x23] = "RMBA",
    [0x2E] = "WDBLI",
    [0x3d] = "WMBA",
    [0x26] = "Set Data Rates",
    [0x2c] = "Dynamically define Data Identifier",

    [0x14] = "Clear Diagnostic Information",
    [0x13] = "Read Diagnostic Trouble Code",
    [0x12] = "Read Freeze Frame Data",
    [0x18] = "Read DTCs by Status",
    [0x17] = "Read Status of DTCs",

    [0x30] = "Input Output Control by Local ID",
    [0x2F] = "Input Output Control by Common ID",

    [0x31] = "Start Routine by Local Identifier",

    [0x34] = "Request Download",
    [0x35] = "Request Upload",
    [0x36] = "Transfer Data",
    [0x37] = "Request Transfer Exit",

    [0x7F] = "", --Negative Response
}

local nrc_description = {
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

local positive_response_mask = 0x40
local negative_response = 0x7F

-- fields
local length = ProtoField.new("Payload Length", "uds_vag.length", ftypes.UINT8,nil, base.DEC)
local sid = ProtoField.uint8("uds_vag.sid", "Service ID", base.HEX, sid_dict_description)
local sid_ispos = ProtoField.uint8("uds_vag.is_positive", "Pos. Res; Service ID", base.HEX, sid_dict_description)
local sid_isneg = ProtoField.uint8("uds_vag.is_negative", "Negative Response", base.HEX, sid_dict_description)
local data = ProtoField.new("Data", "uds_vag.data", ftypes.BYTES)
local nrc = ProtoField.uint8("uds_vag.nrc", "Negative Response Code", base.HEX, nrc_description)

-- declare dissector
local uds_dissector = Proto.new("uds_vag", "UDS (VAG)")

uds_dissector.fields = {
	length,
    sid,
    sid_ispos,
    sid_isneg,
    nrc,
    data,
}

pktState = {}

function uds_dissector.dissector(tvbuf,pktinfo,root)
    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("UDS")
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(uds_dissector, tvbuf:range(0,pktlen))
    tree:add(length, tvbuf:len())

    local cur_sid = tvbuf:range(0,1):uint()
    pktlen = tvbuf:len()

    if cur_sid == negative_response then    -- negative response
        tree:add(sid_isneg, tvbuf:range(0,1))
        tree:add(sid, tvbuf:range(1,1))
        tree:add(nrc, tvbuf:range(2,1))
        pktinfo.cols.info = "NOK"
    elseif bit32.btest(cur_sid, positive_response_mask) then -- positive response (bit 6 is set)
        tree:add(sid_ispos, tvbuf:range(0,1))
        tree:add(data, tvbuf:range(0,pktlen))
        pktinfo.cols.info = "OK" .. " " .. tostring(tvbuf:range(0,pktlen))
    elseif sid_dict_description[cur_sid] ~= nil then    -- normal request
        tree:add(sid, tvbuf:range(0,1))
        tree:add(data, tvbuf:range(0,pktlen))
		pktinfo.cols.info = sid_dict_description[cur_sid]
    end
end

--Asign to protocol 0x94: ISO15765
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0x94, uds_dissector)
