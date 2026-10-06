/**
 * OneNet 安全鉴权（文档 doc/1464）纯 JS 实现
 * 不依赖 Node crypto / btoa / atob，H5、App、小程序通用
 *
 * sign = base64(hmac_sha1(base64decode(accessKey), StringForSignature))
 * StringForSignature = et + "\n" + method + "\n" + res + "\n" + version
 */

const B64_CHARS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

function base64Encode(bytes) {
	let str = ''
	const len = bytes.length
	for (let i = 0; i < len; i += 3) {
		const b1 = bytes[i]
		const b2 = i + 1 < len ? bytes[i + 1] : 0
		const b3 = i + 2 < len ? bytes[i + 2] : 0
		const c1 = b1 >> 2
		const c2 = ((b1 & 0x03) << 4) | (b2 >> 4)
		const c3 = ((b2 & 0x0f) << 2) | (b3 >> 6)
		const c4 = b3 & 0x3f
		str += B64_CHARS[c1] + B64_CHARS[c2]
		str += i + 1 < len ? B64_CHARS[c3] : '='
		str += i + 2 < len ? B64_CHARS[c4] : '='
	}
	return str
}

function base64Decode(str) {
	const clean = String(str).replace(/[^A-Za-z0-9+/=]/g, '')
	const lookup = {}
	for (let i = 0; i < B64_CHARS.length; i++) {
		lookup[B64_CHARS.charAt(i)] = i
	}
	const bytes = []
	for (let i = 0; i < clean.length; i += 4) {
		const c1 = lookup[clean.charAt(i)]
		const c2 = lookup[clean.charAt(i + 1)]
		const c3 = lookup[clean.charAt(i + 2)]
		const c4 = lookup[clean.charAt(i + 3)]
		bytes.push((c1 << 2) | (c2 >> 4))
		if (clean.charAt(i + 2) !== '=') {
			bytes.push(((c2 & 0x0f) << 4) | (c3 >> 2))
		}
		if (clean.charAt(i + 3) !== '=') {
			bytes.push(((c3 & 0x03) << 6) | c4)
		}
	}
	return bytes
}

function utf8Bytes(str) {
	const out = []
	for (let i = 0; i < str.length; i++) {
		let c = str.charCodeAt(i)
		if (c < 0x80) {
			out.push(c)
		} else if (c < 0x800) {
			out.push(0xc0 | (c >> 6), 0x80 | (c & 0x3f))
		} else if (c >= 0xd800 && c <= 0xdbff) {
			const c2 = str.charCodeAt(++i)
			c = 0x10000 + ((c & 0x3ff) << 10) + (c2 & 0x3ff)
			out.push(0xf0 | (c >> 18), 0x80 | ((c >> 12) & 0x3f), 0x80 | ((c >> 6) & 0x3f), 0x80 | (c & 0x3f))
		} else {
			out.push(0xe0 | (c >> 12), 0x80 | ((c >> 6) & 0x3f), 0x80 | (c & 0x3f))
		}
	}
	return out
}

function rotl(x, n) {
	return (x << n) | (x >>> (32 - n))
}

/** SHA-1，输入/输出均为字节数组 */
function sha1Bytes(bytes) {
	const n = bytes.length
	const total = (((n + 8) >> 6) + 1) * 64
	const buf = new Array(total).fill(0)
	for (let i = 0; i < n; i++) {
		buf[i] = bytes[i]
	}
	buf[n] = 0x80
	const bits = n * 8
	buf[total - 4] = (bits >>> 24) & 0xff
	buf[total - 3] = (bits >>> 16) & 0xff
	buf[total - 2] = (bits >>> 8) & 0xff
	buf[total - 1] = bits & 0xff

	let h0 = 0x67452301
	let h1 = 0xefcdab89
	let h2 = 0x98badcfe
	let h3 = 0x10325476
	let h4 = 0xc3d2e1f0
	const w = new Array(80)

	for (let off = 0; off < total; off += 64) {
		for (let i = 0; i < 16; i++) {
			w[i] = (buf[off + i * 4] << 24) |
				(buf[off + i * 4 + 1] << 16) |
				(buf[off + i * 4 + 2] << 8) |
				buf[off + i * 4 + 3]
		}
		for (let i = 16; i < 80; i++) {
			w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1)
		}
		let a = h0
		let b = h1
		let c = h2
		let d = h3
		let e = h4
		for (let i = 0; i < 80; i++) {
			let f
			let k
			if (i < 20) {
				f = (b & c) | (~b & d)
				k = 0x5a827999
			} else if (i < 40) {
				f = b ^ c ^ d
				k = 0x6ed9eba1
			} else if (i < 60) {
				f = (b & c) | (b & d) | (c & d)
				k = 0x8f1bbcdc
			} else {
				f = b ^ c ^ d
				k = 0xca62c1d6
			}
			const t = (rotl(a, 5) + f + e + k + w[i]) | 0
			e = d
			d = c
			c = rotl(b, 30)
			b = a
			a = t
		}
		h0 = (h0 + a) | 0
		h1 = (h1 + b) | 0
		h2 = (h2 + c) | 0
		h3 = (h3 + d) | 0
		h4 = (h4 + e) | 0
	}

	const out = []
	;[h0, h1, h2, h3, h4].forEach(h => {
		out.push((h >>> 24) & 0xff, (h >>> 16) & 0xff, (h >>> 8) & 0xff, h & 0xff)
	})
	return out
}

function hmacSha1(keyBytes, msgBytes) {
	const blockSize = 64
	let key = keyBytes
	if (key.length > blockSize) {
		key = sha1Bytes(key)
	}
	key = key.slice()
	while (key.length < blockSize) {
		key.push(0)
	}
	const ipad = key.map(b => b ^ 0x36)
	const opad = key.map(b => b ^ 0x5c)
	const inner = sha1Bytes(ipad.concat(msgBytes))
	return sha1Bytes(opad.concat(inner))
}

/**
 * 生成产品级 authorization
 * @param {String} productId 产品ID
 * @param {String} accessKey 产品 AccessKey（base64）
 * @param {Number} ttlSeconds token 有效期（秒）
 * @returns authorization 请求头的值
 */
export function generateProductAuthorization(productId, accessKey, ttlSeconds = 3600) {
	const version = '2022-05-01'
	const method = 'sha1'
	const res = 'products/' + productId
	const et = Math.floor(Date.now() / 1000) + ttlSeconds
	const stringForSignature = et + '\n' + method + '\n' + res + '\n' + version

	const keyBytes = base64Decode(accessKey)
	const signBytes = hmacSha1(keyBytes, utf8Bytes(stringForSignature))
	const sign = base64Encode(signBytes)

	return 'version=' + version +
		'&res=' + encodeURIComponent(res) +
		'&et=' + et +
		'&method=' + method +
		'&sign=' + encodeURIComponent(sign)
}
