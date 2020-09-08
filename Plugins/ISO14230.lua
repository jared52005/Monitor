-- LUA bitwise operations: https://www.lua.org/manual/5.2/manual.html#6.7
-- constants
local fmtbyte_addressing = {
    [0x00] = "No Address Information",
    [0x01] = "CARB Mode",
    [0x02] = "Physical addressing",
    [0x03] = "Functional addressing",
}

-- fields
local iso_fmt_address = ProtoField.uint8("iso14230.fmt.address", "Addressing mode", base.HEX, fmtbyte_addressing, 0xC0)
local iso_fmt_len = ProtoField.uint8("iso14230.fmt.len", "Payload length", base.DEC, nil, 0x3F)
local iso_length = ProtoField.uint8("iso14230.length", "Payload Length", base.DEC)
local iso_tgt = ProtoField.uint8("iso14230.tgt", "Target Address", base.HEX)
local iso_src = ProtoField.uint8("iso14230.src", "Source Address", base.HEX)
local iso_data = ProtoField.new("Data", "iso14230.data", ftypes.BYTES) --Data for next layer
local iso_cs = ProtoField.uint8("iso14230.cs", "Checksum", base.HEX)
local iso_cs_valid = ProtoField.string("iso14230.cs.status", "Checksum status")

-- declare dissector
local iso14230_dissector = Proto.new("iso14230", "ISO14230")

iso14230_dissector.fields = {
    iso_fmt_address,
    iso_fmt_len,
	iso_length,
    iso_data,
    iso_tgt,
    iso_src,
    iso_cs,
    iso_cs_valid,
}

function iso14230_dissector.dissector(tvbuf,pktinfo,root)
    -- set the protocol column to show our protocol name
    pktinfo.cols.protocol:set("ISO14230")
    local pktlen = tvbuf:reported_length_remaining()
    local tree = root:add(iso14230_dissector, tvbuf:range(0,pktlen))

    -- Parse header and LEN byte
    local fmt = tvbuf:range(0,1):uint()
    local addressing = bit32.rshift(bit32.band(fmt, 0xC0), 6)
    tree:add(iso_fmt_address, tvbuf:range(0,1))
    local fmt_len = bit32.band(fmt, 0x3F) --Get Length byte from FMT

    -- Parse physical addressing (if included)
    local parserPosition = 1
    if(addressing == 0x02) then
        tree:add(iso_tgt, tvbuf:range(parserPosition,1))
        parserPosition = parserPosition + 1
        tree:add(iso_src, tvbuf:range(parserPosition,1))
        parserPosition = parserPosition + 1
    end

    -- Parse length byte (if included)
    local length
    if(fmt_len == 0) then
        tree:add(iso_length, tvbuf:range(parserPosition,1))
        length = tvbuf:range(parserPosition,1):uint()
        parserPosition = parserPosition + 1
    else
        tree:add(iso_fmt_len, tvbuf:range(0,1))
        length = fmt_len
    end

    -- calculate checksum
    local cs = 0
    for i=0,pktlen-2,1
    do
        cs = cs + tvbuf:range(i,1):uint()
    end
    cs = bit32.band(cs, 0xFF)
    stored_cs = tvbuf:range(pktlen - 1,1):uint()
    tree:add(iso_cs, tvbuf:range(pktlen - 1,1))
    if cs == stored_cs then
        tree:add(iso_cs_valid, "Good")
    else
        tree:add(iso_cs_valid, "Bad")
    end 

    -- Call top level protocol. In this case KWP2000
    local kwp = Dissector.get("kwp2k")
    if kwp ~= nil then
        kwp:call(tvbuf:range(parserPosition,length):tvb(), pktinfo, root)
    else
        tree:add(iso_data, tvbuf:range(parserPosition,length))
    end
end

--Asign to protocol 0x91: ISO14230
local ipProtocol = DissectorTable.get("ip.proto")
ipProtocol:add(0x91, iso14230_dissector)
