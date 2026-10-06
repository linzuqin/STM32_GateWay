<script>
	import store from './utils/onenet/store.js'

	export default {
		onLaunch: function() {
			console.log('App Launch')
			// 全局启动 OneNet 数据轮询，tabBar 页面之间切换不会中断
			store.start()
		},
		onShow: function() {
			console.log('App Show')
			store.start()
		},
		onHide: function() {
			console.log('App Hide')
			store.stop()
		}
	}
</script>

<style>
	/* 每个页面公共样式 */
	page {
		background: #f4f6fa;
	}

	.page {
		min-height: 100vh;
		padding: 24rpx;
		box-sizing: border-box;
		background: #f4f6fa;
	}

	.card {
		background: #ffffff;
		border-radius: 18rpx;
		box-shadow: 0 6rpx 20rpx rgba(31, 41, 55, 0.05);
	}

	.section-title {
		display: flex;
		flex-direction: row;
		align-items: center;
		margin: 32rpx 4rpx 16rpx;
	}

	.title-bar {
		width: 8rpx;
		height: 28rpx;
		border-radius: 4rpx;
		background: #2b7fff;
		margin-right: 12rpx;
	}

	.title-text {
		font-size: 30rpx;
		font-weight: 600;
		color: #1f2937;
	}

	.mono {
		font-family: -apple-system, Menlo, Consolas, monospace;
	}

	.footer-tip {
		display: flex;
		justify-content: center;
		padding: 36rpx 0 20rpx;
	}

	.footer-tip text {
		font-size: 22rpx;
		color: #b0b6bf;
	}
</style>
